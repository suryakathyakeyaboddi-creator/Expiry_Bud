import { afterEach, beforeEach, describe, expect, it } from "vitest";
import { createTestContext } from "./testHelpers.js";

describe("settings API", () => {
  let context;

  beforeEach(async () => {
    context = await createTestContext();
  });

  afterEach(async () => {
    await context.cleanup();
  });

  it("updates and persists the alert threshold", async () => {
    const response = await context.api
      .put("/api/settings")
      .send({ alertThresholdDays: 14 })
      .expect(200);
    expect(response.body.alertThresholdDays).toBe(14);
    expect(await context.readData("settings.txt")).toContain("alert_threshold_days=14");
  });

  it.each([-1, 366, 2.5, "not-a-number"])('rejects invalid threshold "%s"', async (value) => {
    await context.api
      .put("/api/settings")
      .send({ alertThresholdDays: value })
      .expect(400);
  });
});
