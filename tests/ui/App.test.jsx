// @vitest-environment jsdom
import { cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import { afterEach, describe, expect, it, vi } from "vitest";
import App from "../../dashboard/client/src/App.jsx";

const snapshot = {
  today: "18-11-2026",
  archivedCount: 0,
  counts: { active: 1, history: 1, safe: 0, expiring: 1, expired: 0 },
  alerts: [
    {
      id: "P000001",
      name: "Almond Milk",
      category: "Dairy",
      expiryDate: "25-11-2026",
      quantity: 2,
      unit: "L",
      unitPrice: 80,
      daysRemaining: 7,
      status: "Expiring"
    }
  ],
  products: [
    {
      id: "P000001",
      name: "Almond Milk",
      category: "Dairy",
      manufactureDate: "01-11-2026",
      expiryDate: "25-11-2026",
      quantity: 2,
      unit: "L",
      unitPrice: 80,
      daysRemaining: 7,
      status: "Expiring"
    }
  ],
  history: [
    {
      id: "P000000",
      name: "Bread",
      category: "Food",
      expiryDate: "10-11-2026",
      quantity: 1,
      unit: "item",
      unitPrice: 30,
      outcome: "Consumed",
      outcomeDate: "09-11-2026"
    }
  ],
  analytics: {
    completedItems: 1,
    wastedItems: 0,
    wasteRatePercent: 0,
    estimatedLoss: 0,
    mostWastedCategory: null,
    byCategory: {},
    byMonth: {}
  },
  categories: ["Food", "Dairy"],
  settings: { alertThresholdDays: 7 }
};

function renderApp() {
  render(
          <App />
  );
}

describe("Expiry Bud dashboard", () => {
  afterEach(() => {
    cleanup();
    vi.restoreAllMocks();
  });

  it("loads the overview and exposes every dashboard section", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => snapshot })
    );
    renderApp();
    expect(await screen.findByText("Your household shelf today")).toBeInTheDocument();
    expect(screen.getByText("Almond Milk")).toBeInTheDocument();

    for (const section of ["Products", "History", "Analytics", "Settings"]) {
      fireEvent.click(screen.getByRole("button", { name: section }));
      await waitFor(() => expect(screen.getByRole("heading", { name: new RegExp(section, "i") })).toBeInTheDocument());
    }
  });

  it("opens the add-product form from the header", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => snapshot })
    );
    renderApp();
    await screen.findByText("Your household shelf today");
    fireEvent.click(screen.getByRole("button", { name: "Add product" }));
    expect(screen.getByRole("heading", { name: "Add a household product" })).toBeInTheDocument();
    expect(screen.getByLabelText("Product name")).toBeInTheDocument();
  });

  it("shows readable analytics details and monthly waste", async () => {
    const analyticsSnapshot = {
      ...snapshot,
      analytics: {
        completedItems: 5,
        wastedItems: 2,
        wasteRatePercent: 40,
        estimatedLoss: 146.5,
        mostWastedCategory: "Dairy",
        byCategory: { Dairy: { wastedItems: 2, estimatedLoss: 146.5 } },
        byMonth: { "2026-11": { wastedItems: 2, estimatedLoss: 146.5 } }
      }
    };
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => analyticsSnapshot })
    );
    renderApp();
    await screen.findByText("Your household shelf today");

    fireEvent.click(screen.getByRole("button", { name: "Analytics" }));

    expect(screen.getByRole("heading", { name: "Waste by category" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Monthly waste" })).toBeInTheDocument();
    expect(screen.getByText("Most wasted category")).toBeInTheDocument();
    expect(screen.getByText("2026-11")).toBeInTheDocument();
    expect(screen.getByRole("button", { name: "Analytics" })).toHaveAttribute("aria-current", "page");
  });

  it("shows settings guidance, saved categories, and a live threshold preview", async () => {
    vi.stubGlobal(
      "fetch",
      vi.fn().mockResolvedValue({ ok: true, status: 200, json: async () => snapshot })
    );
    renderApp();
    await screen.findByText("Your household shelf today");

    fireEvent.click(screen.getByRole("button", { name: "Settings" }));

    expect(screen.getByRole("heading", { name: "Alert window" })).toBeInTheDocument();
    expect(screen.getByRole("heading", { name: "Product categories" })).toBeInTheDocument();
    expect(screen.getByLabelText("Saved product categories")).toHaveTextContent("Food");
    expect(screen.getByText("7 days before expiry")).toBeInTheDocument();

    fireEvent.change(screen.getByLabelText("Days before expiry"), { target: { value: "14" } });
    expect(screen.getByText("14 days before expiry")).toBeInTheDocument();
  });
});
