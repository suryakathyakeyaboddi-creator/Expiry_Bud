import {
  Badge,
  Empty,
} from "../ui.jsx";

export function HistoryTable({ history }) {
  if (!history.length) {
    return (
      <Empty>
        <h3>No history yet</h3>
        <p>Consumed, discarded, and expired products appear here.</p>
      </Empty>
    );
  }
  return (
    <div className="table-scroll">
      <table aria-label="Product history">
        <thead>
          <tr>
            <th>Product</th>
            <th>Category</th>
            <th>Expiry</th>
            <th>Outcome</th>
            <th>Outcome date</th>
            <th>Value</th>
          </tr>
        </thead>
        <tbody>
          {[...history].reverse().map((product) => (
            <tr key={`${product.id}-${product.outcomeDate}`}>
              <td>
                <strong>{product.name}</strong>
                <span className="muted row-id">{product.id}</span>
              </td>
              <td>{product.category}</td>
              <td>{product.expiryDate}</td>
              <td>
                <Badge variant="outline">{product.outcome}</Badge>
              </td>
              <td>{product.outcomeDate ?? "-"}</td>
              <td>
                {product.unitPrice == null
                  ? "Not recorded"
                  : `Rs. ${(product.quantity * product.unitPrice).toFixed(2)}`}
              </td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
