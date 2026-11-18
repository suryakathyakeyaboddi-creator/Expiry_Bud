import {
  Badge,
  Button,
  Empty,
} from "../ui.jsx";

function statusClass(status) {
  return `status-badge status-${status.toLowerCase()}`;
}

export function ProductTable({ products, onEdit, onOutcome, onDelete, busy }) {
  if (!products.length) {
    return (
      <Empty>
        <h3>No products here</h3>
        <p>Adjust the filter or add the first household product.</p>
      </Empty>
    );
  }

  return (
    <div className="table-scroll">
      <table aria-label="Active products">
        <thead>
          <tr>
            <th>Product</th>
            <th>Category</th>
            <th>Expiry</th>
            <th>Status</th>
            <th>Quantity</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody>
          {products.map((product) => (
            <tr key={product.id}>
              <td>
                <strong>{product.name}</strong>
                <span className="muted row-id">{product.id}</span>
              </td>
              <td>{product.category}</td>
              <td>
                {product.expiryDate}
                <span className="muted row-id">
                  {product.daysRemaining === 0
                    ? "Today"
                    : `${Math.abs(product.daysRemaining)} day${Math.abs(product.daysRemaining) === 1 ? "" : "s"} ${product.daysRemaining < 0 ? "ago" : "left"}`}
                </span>
              </td>
              <td>
                <Badge className={statusClass(product.status)} variant="outline">
                  {product.status}
                </Badge>
              </td>
              <td>
                {product.quantity} {product.unit}
              </td>
              <td>
                <div className="row-actions">
                  <Button size="sm" variant="ghost" onClick={() => onEdit(product)} disabled={busy}>
                    Edit
                  </Button>
                  <Button
                    size="sm"
                    variant="outline"
                    onClick={() => onOutcome(product, "Consumed")}
                    disabled={busy}
                  >
                    Consumed
                  </Button>
                  <Button
                    size="sm"
                    variant="outline"
                    onClick={() => onOutcome(product, "Discarded")}
                    disabled={busy}
                  >
                    Discard
                  </Button>
                  <Button size="sm" variant="ghost" onClick={() => onDelete(product)} disabled={busy}>
                    Delete
                  </Button>
                </div>
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
