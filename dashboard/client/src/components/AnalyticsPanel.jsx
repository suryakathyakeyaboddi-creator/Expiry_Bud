import { Card, CardContent, CardHeader, CardTitle } from "../ui.jsx";
import { MetricCard } from "./MetricCard.jsx";

export function AnalyticsPanel({ analytics }) {
  const categoryRows = Object.entries(analytics.byCategory ?? {});
  const monthRows = Object.entries(analytics.byMonth ?? {}).sort(([left], [right]) => right.localeCompare(left));
  const maximum = Math.max(1, ...categoryRows.map(([, value]) => value.wastedItems));
  const palette = ["#d9584f", "#e2943b", "#3d83a8", "#3f8b68", "#846da8"];

  return (
    <div className="stack">
      <div className="metric-grid">
        <MetricCard label="Completed" value={analytics.completedItems} note="lifecycle records" />
        <MetricCard label="Wasted" value={analytics.wastedItems} note="discarded or expired" tone="red" />
        <MetricCard
          label="Waste rate"
          value={`${analytics.wasteRatePercent.toFixed(1)}%`}
          note="of completed items"
          tone="orange"
        />
        <MetricCard
          label="Estimated loss"
          value={`Rs. ${analytics.estimatedLoss.toFixed(2)}`}
          note="priced waste only"
          tone="blue"
        />
      </div>
      <section className="analytics-story" aria-label="Waste summary">
        <div className="insight-block insight-coral">
          <span>Most wasted category</span>
          <strong>{analytics.mostWastedCategory || "None yet"}</strong>
          <p>{analytics.wastedItems ? "Start here when planning your next shop." : "No category needs attention."}</p>
        </div>
        <div className="insight-block insight-blue">
          <span>Consumed</span>
          <strong>{analytics.completedItems - analytics.wastedItems} items</strong>
          <p>Completed without becoming household waste.</p>
        </div>
        <div className="insight-block insight-yellow">
          <span>Months recorded</span>
          <strong>{monthRows.length}</strong>
          <p>Monthly waste history available for comparison.</p>
        </div>
      </section>

      <div className="analytics-grid">
      <Card className="analytics-card analytics-card-wide">
        <CardHeader>
          <CardTitle>Waste by category</CardTitle>
        </CardHeader>
        <CardContent>
          {!categoryRows.length ? (
            <div className="empty-panel">
              <strong>No waste recorded</strong>
              <p>Discarded and expired products will appear here.</p>
            </div>
          ) : (
            <div className="bar-chart">
              {categoryRows.map(([category, values], index) => (
                <div className="bar-row" key={category} style={{ "--bar-color": palette[index % palette.length] }}>
                  <strong className="bar-label">{category}</strong>
                  <div className="bar-track">
                    <div
                      className="bar-fill"
                      style={{ width: `${(values.wastedItems / maximum) * 100}%` }}
                    />
                  </div>
                  <strong className="bar-count">{values.wastedItems}</strong>
                  <span className="bar-loss">Rs. {values.estimatedLoss.toFixed(2)}</span>
                </div>
              ))}
            </div>
          )}
        </CardContent>
      </Card>

      <Card className="analytics-card">
        <CardHeader>
          <CardTitle>Monthly waste</CardTitle>
        </CardHeader>
        <CardContent>
          {!monthRows.length ? (
            <div className="empty-panel empty-panel-blue">
              <strong>No monthly pattern yet</strong>
              <p>Completed waste records will appear here by month.</p>
            </div>
          ) : (
            <div className="month-list">
              {monthRows.map(([month, values]) => (
                <div className="month-row" key={month}>
                  <div><strong>{month}</strong><span>{values.wastedItems} wasted</span></div>
                  <strong>Rs. {values.estimatedLoss.toFixed(2)}</strong>
                </div>
              ))}
            </div>
          )}
        </CardContent>
      </Card>
      </div>
    </div>
  );
}
