import { Icon } from "../icons.jsx";
import { Card, CardContent, CardHeader, CardTitle } from "../ui.jsx";

export function MetricCard({ label, value, note, tone = "green" }) {
  const icons = { Active: "products", Safe: "check", Expiring: "bell", History: "history", Completed: "check", Wasted: "products", "Waste rate": "analytics", "Estimated loss": "wallet" };
  return (
    <Card className={`metric-card tone-${tone}`}>
      <CardHeader>
        <CardTitle>{label}</CardTitle><span className="metric-icon"><Icon name={icons[label]} /></span>
      </CardHeader>
      <CardContent>
        <strong className={`metric-value ${String(value).length > 7 ? "metric-value-compact" : ""}`}>{value}</strong>
        <span className="metric-note">{note}</span>
      </CardContent>
    </Card>
  );
}
