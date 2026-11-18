import { parse } from "csv-parse/sync";
import { stringify } from "csv-stringify/sync";
import { mkdir, readFile, rename, rm, writeFile } from "node:fs/promises";
import path from "node:path";

const PRODUCT_COLUMNS = [
  "id",
  "name",
  "category",
  "manufacture_date",
  "expiry_date",
  "quantity",
  "unit",
  "unit_price",
  "date_added",
  "outcome",
  "outcome_date"
];
const DEFAULT_CATEGORIES = ["Food", "Medicine", "Cosmetics", "Dairy", "Grocery", "Household"];

function fromCsvRecord(record) {
  return {
    id: record.id,
    name: record.name,
    category: record.category,
    manufactureDate: record.manufacture_date || null,
    expiryDate: record.expiry_date,
    quantity: Number(record.quantity),
    unit: record.unit,
    unitPrice: record.unit_price === "" ? null : Number(record.unit_price),
    dateAdded: record.date_added,
    outcome: record.outcome,
    outcomeDate: record.outcome_date || null
  };
}

function toCsvRecord(product) {
  return {
    id: product.id,
    name: product.name,
    category: product.category,
    manufacture_date: product.manufactureDate ?? "",
    expiry_date: product.expiryDate,
    quantity: product.quantity,
    unit: product.unit,
    unit_price: product.unitPrice ?? "",
    date_added: product.dateAdded,
    outcome: product.outcome,
    outcome_date: product.outcomeDate ?? ""
  };
}

export class DataStore {
  constructor(dataDirectory) {
    this.dataDirectory = dataDirectory;
    this.queue = Promise.resolve();
  }

  path(fileName) {
    return path.join(this.dataDirectory, fileName);
  }

  async ensureFiles() {
    await mkdir(this.dataDirectory, { recursive: true });
    await this.ensureFile("products.csv", `${PRODUCT_COLUMNS.join(",")}\n`);
    await this.ensureFile("history.csv", `${PRODUCT_COLUMNS.join(",")}\n`);
    await this.ensureFile("categories.csv", `${DEFAULT_CATEGORIES.join("\n")}\n`);
    await this.ensureFile("settings.txt", "alert_threshold_days=7\n");
  }

  async ensureFile(fileName, content) {
    try {
      await readFile(this.path(fileName));
    } catch (error) {
      if (error.code !== "ENOENT") throw error;
      await writeFile(this.path(fileName), content, "utf8");
    }
  }

  async loadProducts(fileName) {
    const source = await readFile(this.path(fileName), "utf8");
    if (!source.trim()) return [];
    return parse(source, {
      columns: true,
      skip_empty_lines: true,
      bom: true
    }).map(fromCsvRecord);
  }

  async loadState() {
    await this.ensureFiles();
    const [products, history, categoryText, settingsText] = await Promise.all([
      this.loadProducts("products.csv"),
      this.loadProducts("history.csv"),
      readFile(this.path("categories.csv"), "utf8"),
      readFile(this.path("settings.txt"), "utf8")
    ]);
    const categories = categoryText
      .split(/\r?\n/)
      .map((category) => category.trim())
      .filter(Boolean);
    const thresholdMatch = /^alert_threshold_days=(\d+)$/m.exec(settingsText);
    return {
      products,
      history,
      categories: categories.length ? categories : [...DEFAULT_CATEGORIES],
      settings: { alertThresholdDays: thresholdMatch ? Number(thresholdMatch[1]) : 7 }
    };
  }

  async atomicWrite(fileName, content) {
    const destination = this.path(fileName);
    const temporary = `${destination}.tmp`;
    await writeFile(temporary, content, "utf8");
    try {
      await rename(temporary, destination);
    } catch (error) {
      if (error.code !== "EEXIST" && error.code !== "EPERM") throw error;
      await rm(destination, { force: true });
      await rename(temporary, destination);
    }
  }

  async saveState(state) {
    const productCsv = stringify(state.products.map(toCsvRecord), {
      header: true,
      columns: PRODUCT_COLUMNS
    });
    const historyCsv = stringify(state.history.map(toCsvRecord), {
      header: true,
      columns: PRODUCT_COLUMNS
    });
    await this.atomicWrite("products.csv", productCsv);
    await this.atomicWrite("history.csv", historyCsv);
    await this.atomicWrite("categories.csv", `${state.categories.join("\n")}\n`);
    await this.atomicWrite(
      "settings.txt",
      `alert_threshold_days=${state.settings.alertThresholdDays}\n`
    );
  }

  async read(handler) {
    await this.queue;
    return handler(await this.loadState());
  }

  async mutate(handler) {
    let release;
    const previous = this.queue;
    this.queue = new Promise((resolve) => {
      release = resolve;
    });
    await previous;
    try {
      const state = await this.loadState();
      const result = await handler(state);
      await this.saveState(state);
      return result;
    } finally {
      release();
    }
  }
}
