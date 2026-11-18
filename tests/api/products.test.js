import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestContext, validProduct } from "./testHelpers.js";

describe("products API", () => {
  let context;

  beforeEach(async () => {
    context = await createTestContext();
  });

  afterEach(async () => {
    await context.cleanup();
  });

  it("creates a product and persists it to products.csv", async () => {
    const response = await context.api.post("/api/products").send(validProduct).expect(201);
    expect(response.body).toMatchObject({
      id: "P000001",
      name: "Almond Milk",
      category: "Dairy",
      outcome: "Active"
    });
    expect(await context.readData("products.csv")).toContain("P000001,Almond Milk,Dairy");
  });

  it.each([
    [{ ...validProduct, name: "" }, "name is required"],
    [{ ...validProduct, category: "Unknown" }, "Unknown category"],
    [{ ...validProduct, quantity: 0 }, "quantity must be greater"],
    [{ ...validProduct, expiryDate: "01-11-2026" }, "after manufactureDate"],
    [{ ...validProduct, expiryDate: "31-02-2026" }, "invalid"]
  ])("rejects invalid input without creating a record", async (payload, message) => {
    const response = await context.api.post("/api/products").send(payload).expect(400);
    expect(response.body.error).toContain(message);
    expect(await context.readData("products.csv")).not.toContain("Almond Milk");
  });

  it("updates a product and reports an unknown product", async () => {
    const created = await context.api.post("/api/products").send(validProduct).expect(201);
    const updated = await context.api
      .put(`/api/products/${created.body.id}`)
      .send({ name: "Oat Milk", quantity: 3 })
      .expect(200);
    expect(updated.body).toMatchObject({ name: "Oat Milk", quantity: 3, unit: "L" });
    await context.api.put("/api/products/P999999").send({ name: "None" }).expect(404);
  });

  it("searches and filters products by calculated status", async () => {
    await context.api.post("/api/products").send(validProduct).expect(201);
    await context.api
      .post("/api/products")
      .send({
        ...validProduct,
        name: "Face Cream",
        category: "Cosmetics",
        manufactureDate: null,
        expiryDate: "01-01-2027"
      })
      .expect(201);
    const search = await context.api.get("/api/products?search=milk").expect(200);
    expect(search.body).toHaveLength(1);
    const expiring = await context.api.get("/api/products?status=expiring").expect(200);
    expect(expiring.body.map((product) => product.name)).toEqual(["Almond Milk"]);
    const safe = await context.api.get("/api/products?status=safe").expect(200);
    expect(safe.body.map((product) => product.name)).toEqual(["Face Cream"]);
  });

  it("moves consumed and discarded products to history", async () => {
    const created = await context.api.post("/api/products").send(validProduct).expect(201);
    await context.api
      .post(`/api/products/${created.body.id}/outcome`)
      .send({ outcome: "Consumed" })
      .expect(200);
    expect((await context.api.get("/api/products")).body).toHaveLength(0);
    expect((await context.api.get("/api/history")).body[0]).toMatchObject({
      id: created.body.id,
      outcome: "Consumed",
      outcomeDate: "18-11-2026"
    });
  });

  it("validates outcomes and missing records", async () => {
    const created = await context.api.post("/api/products").send(validProduct).expect(201);
    await context.api
      .post(`/api/products/${created.body.id}/outcome`)
      .send({ outcome: "Active" })
      .expect(400);
    await context.api
      .post("/api/products/P999999/outcome")
      .send({ outcome: "Consumed" })
      .expect(404);
  });

  it("deletes an active product and reports unknown IDs", async () => {
    const created = await context.api.post("/api/products").send(validProduct).expect(201);
    await context.api.delete(`/api/products/${created.body.id}`).expect(204);
    expect(await context.readData("products.csv")).not.toContain("Almond Milk");
    await context.api.delete(`/api/products/${created.body.id}`).expect(404);
  });
});
