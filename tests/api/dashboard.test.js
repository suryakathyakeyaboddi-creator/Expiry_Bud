import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestContext, validProduct } from "./testHelpers.js";

describe("dashboard and analytics APIs", () => {
  let context;

  beforeEach(async () => {
    context = await createTestContext();
  });

  afterEach(async () => {
    await context.cleanup();
  });

  it("returns metrics and near-expiry alerts", async () => {
    await context.api.post("/api/products").send(validProduct).expect(201);
    const response = await context.api.get("/api/dashboard").expect(200);
    expect(response.body.counts).toMatchObject({ active: 1, expiring: 1, safe: 0 });
    expect(response.body.alerts[0].name).toBe("Almond Milk");
    expect(response.body.settings.alertThresholdDays).toBe(7);
  });

  it("archives overdue products once and updates CSV files", async () => {
    await context.api
      .post("/api/products")
      .send({ ...validProduct, expiryDate: "17-11-2026" })
      .expect(201);
    const first = await context.api.get("/api/dashboard").expect(200);
    expect(first.body.archivedCount).toBe(1);
    expect(first.body.counts).toMatchObject({ active: 0, history: 1 });
    const second = await context.api.get("/api/dashboard").expect(200);
    expect(second.body.archivedCount).toBe(0);
    expect(await context.readData("history.csv")).toContain("ExpiredUnused");
  });

  it("calculates waste analytics after a discarded outcome", async () => {
    const created = await context.api.post("/api/products").send(validProduct).expect(201);
    await context.api
      .post(`/api/products/${created.body.id}/outcome`)
      .send({ outcome: "Discarded" })
      .expect(200);
    const analytics = await context.api.get("/api/analytics").expect(200);
    expect(analytics.body).toMatchObject({
      completedItems: 1,
      wastedItems: 1,
      wasteRatePercent: 100,
      estimatedLoss: 160,
      mostWastedCategory: "Dairy"
    });
    expect(analytics.body.byMonth["11-2026"].wastedItems).toBe(1);
  });

  it("returns JSON errors for missing routes and malformed bodies", async () => {
    await context.api.get("/api/unknown").expect(404);
    const response = await context.api
      .post("/api/products")
      .set("Content-Type", "application/json")
      .send("{not json")
      .expect(400);
    expect(response.body.error).toBeTruthy();
  });
});
