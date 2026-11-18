// @vitest-environment jsdom
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import { afterEach, describe, expect, it, vi } from "vitest";
import { ProductForm } from "../../dashboard/client/src/components/ProductForm.jsx";

function renderForm(onSubmit) {
  render(
        <ProductForm
          categories={["Food", "Dairy"]}
          onSubmit={onSubmit}
          onCancel={() => {}}
          busy={false}
        />
  );
}

describe("ProductForm", () => {
  afterEach(cleanup);

  it("submits normalized product values", () => {
    const onSubmit = vi.fn();
    renderForm(onSubmit);
    fireEvent.change(screen.getByLabelText("Product name"), { target: { value: "Milk" } });
    fireEvent.change(screen.getByLabelText("Category"), { target: { value: "Dairy" } });
    fireEvent.change(screen.getByLabelText("Expiry date"), { target: { value: "30-11-2026" } });
    fireEvent.change(screen.getByLabelText("Quantity"), { target: { value: "2" } });
    fireEvent.change(screen.getByLabelText("Unit"), { target: { value: "L" } });
    fireEvent.change(screen.getByLabelText("Unit price (Rs.)"), { target: { value: "50" } });
    fireEvent.click(screen.getByRole("button", { name: "Add product" }));
    expect(onSubmit).toHaveBeenCalledWith(
      expect.objectContaining({
        name: "Milk",
        category: "Dairy",
        quantity: 2,
        unit: "L",
        unitPrice: 50,
        manufactureDate: null
      })
    );
  });
});
