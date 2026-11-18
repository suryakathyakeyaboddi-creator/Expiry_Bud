const DAY_MS = 24 * 60 * 60 * 1000;

export class HttpError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}

export function parseDate(value, fieldName = "date") {
  if (typeof value !== "string") {
    throw new HttpError(400, `${fieldName} must use DD-MM-YYYY format`);
  }
  const match = /^(\d{2})-(\d{2})-(\d{4})$/.exec(value);
  if (!match) {
    throw new HttpError(400, `${fieldName} must use DD-MM-YYYY format`);
  }
  const [, dayText, monthText, yearText] = match;
  const day = Number(dayText);
  const month = Number(monthText);
  const year = Number(yearText);
  const date = new Date(Date.UTC(year, month - 1, day));
  if (
    year < 1900 ||
    year > 9999 ||
    date.getUTCFullYear() !== year ||
    date.getUTCMonth() !== month - 1 ||
    date.getUTCDate() !== day
  ) {
    throw new HttpError(400, `${fieldName} is invalid`);
  }
  return date;
}

export function formatDate(date) {
  const day = String(date.getDate()).padStart(2, "0");
  const month = String(date.getMonth() + 1).padStart(2, "0");
  return `${day}-${month}-${date.getFullYear()}`;
}

export function daysRemaining(expiryDate, todayText) {
  const expiry = parseDate(expiryDate, "expiryDate");
  const today = parseDate(todayText, "today");
  return Math.round((expiry.getTime() - today.getTime()) / DAY_MS);
}

export function expiryStatus(product, todayText, thresholdDays) {
  const remaining = daysRemaining(product.expiryDate, todayText);
  if (remaining < 0) return "Expired";
  if (remaining <= thresholdDays) return "Expiring";
  return "Safe";
}

export function estimatedValue(product) {
  if (product.unitPrice === null || product.unitPrice === undefined || product.unitPrice === "") {
    return 0;
  }
  return product.quantity * product.unitPrice;
}

export function enrichProduct(product, todayText, thresholdDays) {
  return {
    ...product,
    daysRemaining: daysRemaining(product.expiryDate, todayText),
    status: expiryStatus(product, todayText, thresholdDays),
    estimatedValue: estimatedValue(product)
  };
}

function requiredText(value, fieldName) {
  if (typeof value !== "string" || !value.trim()) {
    throw new HttpError(400, `${fieldName} is required`);
  }
  if (/\r|\n/.test(value)) {
    throw new HttpError(400, `${fieldName} cannot contain line breaks`);
  }
  return value.trim();
}

function optionalNumber(value, fieldName) {
  if (value === null || value === undefined || value === "") return null;
  const number = Number(value);
  if (!Number.isFinite(number) || number < 0) {
    throw new HttpError(400, `${fieldName} must be zero or greater`);
  }
  return number;
}

export function normalizeProduct(input, categories, existing = {}) {
  const product = {
    ...existing,
    name: requiredText(input.name ?? existing.name, "name"),
    category: requiredText(input.category ?? existing.category, "category"),
    manufactureDate:
      input.manufactureDate === "" || input.manufactureDate === null
        ? null
        : input.manufactureDate ?? existing.manufactureDate ?? null,
    expiryDate: requiredText(input.expiryDate ?? existing.expiryDate, "expiryDate"),
    quantity: Number(input.quantity ?? existing.quantity),
    unit: requiredText(input.unit ?? existing.unit, "unit"),
    unitPrice: optionalNumber(input.unitPrice ?? existing.unitPrice, "unitPrice")
  };

  const category = categories.find(
    (candidate) => candidate.toLowerCase() === product.category.toLowerCase()
  );
  if (!category) throw new HttpError(400, `Unknown category: ${product.category}`);
  product.category = category;

  if (!Number.isFinite(product.quantity) || product.quantity <= 0) {
    throw new HttpError(400, "quantity must be greater than zero");
  }
  const expiry = parseDate(product.expiryDate, "expiryDate");
  if (product.manufactureDate) {
    const manufacture = parseDate(product.manufactureDate, "manufactureDate");
    if (expiry.getTime() <= manufacture.getTime()) {
      throw new HttpError(400, "expiryDate must be after manufactureDate");
    }
  }
  return product;
}

export function nextProductId(state) {
  const greatest = [...state.products, ...state.history].reduce((maximum, product) => {
    const match = /^P(\d+)$/.exec(product.id);
    return match ? Math.max(maximum, Number(match[1])) : maximum;
  }, 0);
  return `P${String(greatest + 1).padStart(6, "0")}`;
}

export function archiveExpired(state, todayText) {
  const active = [];
  let archivedCount = 0;
  for (const product of state.products) {
    if (expiryStatus(product, todayText, state.settings.alertThresholdDays) !== "Expired") {
      active.push(product);
      continue;
    }
    if (!state.history.some((item) => item.id.toLowerCase() === product.id.toLowerCase())) {
      state.history.push({
        ...product,
        outcome: "ExpiredUnused",
        outcomeDate: todayText
      });
      archivedCount += 1;
    }
  }
  state.products = active;
  return archivedCount;
}

export function analyticsSummary(history) {
  const wasted = history.filter(
    (product) => product.outcome === "Discarded" || product.outcome === "ExpiredUnused"
  );
  const byCategory = {};
  const byMonth = {};

  for (const product of wasted) {
    const value = estimatedValue(product);
    byCategory[product.category] ??= { wastedItems: 0, estimatedLoss: 0 };
    byCategory[product.category].wastedItems += 1;
    byCategory[product.category].estimatedLoss += value;

    if (product.outcomeDate) {
      const [, month, year] = product.outcomeDate.split("-");
      const key = `${month}-${year}`;
      byMonth[key] ??= { wastedItems: 0, estimatedLoss: 0 };
      byMonth[key].wastedItems += 1;
      byMonth[key].estimatedLoss += value;
    }
  }

  const mostWastedCategory = Object.entries(byCategory).sort(
    ([leftName, left], [rightName, right]) =>
      right.wastedItems - left.wastedItems || leftName.localeCompare(rightName)
  )[0]?.[0] ?? null;

  return {
    completedItems: history.length,
    wastedItems: wasted.length,
    wasteRatePercent: history.length ? (wasted.length / history.length) * 100 : 0,
    estimatedLoss: wasted.reduce((sum, product) => sum + estimatedValue(product), 0),
    mostWastedCategory,
    byCategory,
    byMonth
  };
}

export function dashboardSnapshot(state, todayText, archivedCount = 0) {
  const products = state.products
    .map((product) => enrichProduct(product, todayText, state.settings.alertThresholdDays))
    .sort((left, right) => left.daysRemaining - right.daysRemaining);
  const counts = products.reduce(
    (summary, product) => {
      summary[product.status.toLowerCase()] += 1;
      return summary;
    },
    { safe: 0, expiring: 0, expired: 0 }
  );
  return {
    today: todayText,
    archivedCount,
    counts: { active: products.length, history: state.history.length, ...counts },
    alerts: products.filter((product) => product.status === "Expiring"),
    products,
    history: state.history,
    analytics: analyticsSummary(state.history),
    categories: state.categories,
    settings: state.settings
  };
}
