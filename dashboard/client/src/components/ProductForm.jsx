import { useEffect, useState } from "react";
import { Button, Card, CardContent, CardHeader, CardTitle, Input, NativeSelect } from "../ui.jsx";

const emptyProduct = {
  name: "",
  category: "",
  manufactureDate: "",
  expiryDate: "",
  quantity: "1",
  unit: "item",
  unitPrice: ""
};

export function ProductForm({ categories, product, onSubmit, onCancel, busy }) {
  const [form, setForm] = useState(emptyProduct);

  useEffect(() => {
    setForm(
      product
        ? {
            name: product.name,
            category: product.category,
            manufactureDate: product.manufactureDate ?? "",
            expiryDate: product.expiryDate,
            quantity: String(product.quantity),
            unit: product.unit,
            unitPrice: product.unitPrice ?? ""
          }
        : { ...emptyProduct, category: categories[0] ?? "" }
    );
  }, [product, categories]);

  const update = (field) => (event) => {
    setForm((current) => ({ ...current, [field]: event.target.value }));
  };

  const submit = (event) => {
    event.preventDefault();
    onSubmit({
      ...form,
      quantity: Number(form.quantity),
      unitPrice: form.unitPrice === "" ? null : Number(form.unitPrice),
      manufactureDate: form.manufactureDate || null
    });
  };

  return (
    <Card className="form-card">
      <CardHeader>
        <CardTitle>{product ? `Edit ${product.name}` : "Add a household product"}</CardTitle>
      </CardHeader>
      <CardContent>
        <form className="product-form" onSubmit={submit}>
          <Input label="Product name" value={form.name} onChange={update("name")} required />
          <label className="field-label">
            Category
            <NativeSelect
              aria-label="Category"
              value={form.category}
              onChange={update("category")}
              options={categories.map((category) => ({ value: category, label: category }))}
              required
            />
          </label>
          <Input
            label="Manufacture date"
            placeholder="DD-MM-YYYY (optional)"
            value={form.manufactureDate}
            onChange={update("manufactureDate")}
          />
          <Input
            label="Expiry date"
            placeholder="DD-MM-YYYY"
            value={form.expiryDate}
            onChange={update("expiryDate")}
            required
          />
          <Input
            label="Quantity"
            type="number"
            min="0.01"
            step="0.01"
            value={form.quantity}
            onChange={update("quantity")}
            required
          />
          <Input label="Unit" value={form.unit} onChange={update("unit")} required />
          <Input
            label="Unit price (Rs.)"
            type="number"
            min="0"
            step="0.01"
            value={form.unitPrice}
            onChange={update("unitPrice")}
          />
          <div className="form-actions">
            <Button type="submit" variant="primary" loading={busy} disabled={busy}>
              {product ? "Save changes" : "Add product"}
            </Button>
            <Button type="button" variant="outline" onClick={onCancel} disabled={busy}>
              Cancel
            </Button>
          </div>
        </form>
      </CardContent>
    </Card>
  );
}
