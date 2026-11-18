import express from "express";
import path from "node:path";
import {
  HttpError,
  analyticsSummary,
  archiveExpired,
  dashboardSnapshot,
  enrichProduct,
  formatDate,
  nextProductId,
  normalizeProduct
} from "./domain.js";
import { DataStore } from "./dataStore.js";

function findProduct(state, id) {
  return state.products.find((product) => product.id.toLowerCase() === id.toLowerCase());
}

export function createApp({ dataDirectory, staticDirectory, todayProvider = () => formatDate(new Date()) }) {
  const app = express();
  const store = new DataStore(dataDirectory);

  app.use(express.json({ limit: "32kb" }));

  app.get("/api/dashboard", async (_request, response) => {
    const today = todayProvider();
    const snapshot = await store.mutate((state) => {
      const archivedCount = archiveExpired(state, today);
      return dashboardSnapshot(state, today, archivedCount);
    });
    response.json(snapshot);
  });

  app.get("/api/products", async (request, response) => {
    const today = todayProvider();
    const products = await store.read((state) => {
      const query = String(request.query.search ?? "").toLowerCase();
      const status = String(request.query.status ?? "").toLowerCase();
      return state.products
        .map((product) => enrichProduct(product, today, state.settings.alertThresholdDays))
        .filter(
          (product) =>
            (!query ||
              product.name.toLowerCase().includes(query) ||
              product.category.toLowerCase().includes(query)) &&
            (!status || product.status.toLowerCase() === status)
        )
        .sort((left, right) => left.daysRemaining - right.daysRemaining);
    });
    response.json(products);
  });

  app.post("/api/products", async (request, response) => {
    const today = todayProvider();
    const product = await store.mutate((state) => {
      const normalized = normalizeProduct(request.body, state.categories);
      const created = {
        ...normalized,
        id: nextProductId(state),
        dateAdded: today,
        outcome: "Active",
        outcomeDate: null
      };
      state.products.push(created);
      return created;
    });
    response.status(201).json(product);
  });

  app.put("/api/products/:id", async (request, response) => {
    const product = await store.mutate((state) => {
      const index = state.products.findIndex(
        (item) => item.id.toLowerCase() === request.params.id.toLowerCase()
      );
      if (index < 0) throw new HttpError(404, "Product not found");
      const updated = normalizeProduct(request.body, state.categories, state.products[index]);
      state.products[index] = updated;
      return updated;
    });
    response.json(product);
  });

  app.delete("/api/products/:id", async (request, response) => {
    await store.mutate((state) => {
      const index = state.products.findIndex(
        (item) => item.id.toLowerCase() === request.params.id.toLowerCase()
      );
      if (index < 0) throw new HttpError(404, "Product not found");
      state.products.splice(index, 1);
    });
    response.status(204).end();
  });

  app.post("/api/products/:id/outcome", async (request, response) => {
    const allowed = ["Consumed", "Discarded"];
    if (!allowed.includes(request.body.outcome)) {
      throw new HttpError(400, "outcome must be Consumed or Discarded");
    }
    const archived = await store.mutate((state) => {
      const product = findProduct(state, request.params.id);
      if (!product) throw new HttpError(404, "Product not found");
      const record = { ...product, outcome: request.body.outcome, outcomeDate: todayProvider() };
      state.products = state.products.filter((item) => item.id !== product.id);
      state.history.push(record);
      return record;
    });
    response.json(archived);
  });

  app.get("/api/history", async (_request, response) => {
    response.json(await store.read((state) => state.history));
  });

  app.get("/api/analytics", async (_request, response) => {
    response.json(await store.read((state) => analyticsSummary(state.history)));
  });

  app.get("/api/categories", async (_request, response) => {
    response.json(await store.read((state) => state.categories));
  });

  app.post("/api/categories", async (request, response) => {
    const name = typeof request.body.name === "string" ? request.body.name.trim() : "";
    if (!name) throw new HttpError(400, "Category name is required");
    if (/\r|\n/.test(name)) throw new HttpError(400, "Category name cannot contain line breaks");
    const category = await store.mutate((state) => {
      if (state.categories.some((item) => item.toLowerCase() === name.toLowerCase())) {
        throw new HttpError(409, "Category already exists");
      }
      state.categories.push(name);
      return name;
    });
    response.status(201).json({ name: category });
  });

  app.delete("/api/categories/:name", async (request, response) => {
    await store.mutate((state) => {
      if (state.categories.length === 1) {
        throw new HttpError(400, "At least one category must remain available");
      }
      const index = state.categories.findIndex(
        (category) => category.toLowerCase() === request.params.name.toLowerCase()
      );
      if (index < 0) throw new HttpError(404, "Category not found");
      const name = state.categories[index];
      const inUse = [...state.products, ...state.history].some(
        (product) => product.category.toLowerCase() === name.toLowerCase()
      );
      if (inUse) throw new HttpError(409, "Category is used by product records");
      state.categories.splice(index, 1);
    });
    response.status(204).end();
  });

  app.get("/api/settings", async (_request, response) => {
    response.json(await store.read((state) => state.settings));
  });

  app.put("/api/settings", async (request, response) => {
    const threshold = Number(request.body.alertThresholdDays);
    if (!Number.isInteger(threshold) || threshold < 0 || threshold > 365) {
      throw new HttpError(400, "alertThresholdDays must be a whole number from 0 to 365");
    }
    const settings = await store.mutate((state) => {
      state.settings.alertThresholdDays = threshold;
      return state.settings;
    });
    response.json(settings);
  });

  if (staticDirectory) {
    app.use(express.static(staticDirectory));
    app.use((request, response, next) => {
      if (request.method !== "GET" || request.path.startsWith("/api")) return next();
      return response.sendFile(path.join(staticDirectory, "index.html"));
    });
  }

  app.use((request, _response, next) => {
    next(new HttpError(404, `Route not found: ${request.method} ${request.path}`));
  });

  app.use((error, _request, response, _next) => {
    const status = error.status ?? (error.type === "entity.parse.failed" ? 400 : 500);
    const message = status === 500 ? "Unexpected server error" : error.message;
    if (status === 500) console.error(error);
    response.status(status).json({ error: message });
  });

  return app;
}
