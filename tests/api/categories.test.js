import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestContext, validProduct } from "./testHelpers.js";

describe("categories API", () => {
  let context;

  beforeEach(async () => {
    context = await createTestContext();
  });

  afterEach(async () => {
    await context.cleanup();
  });

  it("adds and removes an unused category with CSV side effects", async () => {
    await context.api.post("/api/categories").send({ name: "Pet Care" }).expect(201);
    expect((await context.api.get("/api/categories")).body).toContain("Pet Care");
    expect(await context.readData("categories.csv")).toContain("Pet Care");
    await context.api.delete("/api/categories/Pet%20Care").expect(204);
    expect((await context.api.get("/api/categories")).body).not.toContain("Pet Care");
  });

  it("rejects empty and duplicate category names", async () => {
    await context.api.post("/api/categories").send({ name: "" }).expect(400);
    await context.api.post("/api/categories").send({ name: "food" }).expect(409);
  });

  it("does not remove a category used by a product", async () => {
    await context.api.post("/api/products").send(validProduct).expect(201);
    const response = await context.api.delete("/api/categories/Dairy").expect(409);
    expect(response.body.error).toContain("used by product records");
  });

  it("reports missing categories", async () => {
    await context.api.delete("/api/categories/Unknown").expect(404);
  });
});
