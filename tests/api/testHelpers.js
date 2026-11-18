import { mkdtemp, readFile, rm } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import request from "supertest";
import { createApp } from "../../dashboard/server/app.js";

export async function createTestContext(today = "18-11-2026") {
  const dataDirectory = await mkdtemp(path.join(os.tmpdir(), "expirybud-api-"));
  const app = createApp({ dataDirectory, todayProvider: () => today });
  return {
    app,
    api: request(app),
    dataDirectory,
    readData: (fileName) => readFile(path.join(dataDirectory, fileName), "utf8"),
    cleanup: () => rm(dataDirectory, { recursive: true, force: true })
  };
}

export const validProduct = {
  name: "Almond Milk",
  category: "Dairy",
  manufactureDate: "01-11-2026",
  expiryDate: "25-11-2026",
  quantity: 2,
  unit: "L",
  unitPrice: 80
};
