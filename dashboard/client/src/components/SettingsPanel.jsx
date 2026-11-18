import { useState } from "react";
import { Badge, Button, Card, CardContent, CardHeader, CardTitle, Input } from "../ui.jsx";

export function SettingsPanel({ snapshot, onThreshold, onAddCategory, onDeleteCategory, busy }) {
  const [threshold, setThreshold] = useState(String(snapshot.settings.alertThresholdDays));
  const [category, setCategory] = useState("");
  const thresholdValue = Number(threshold);
  const previewDays = Number.isFinite(thresholdValue) ? Math.min(365, Math.max(0, thresholdValue)) : 0;

  const saveThreshold = (event) => {
    event.preventDefault();
    onThreshold(Number(threshold));
  };

  const addCategory = (event) => {
    event.preventDefault();
    onAddCategory(category).then((succeeded) => {
      if (succeeded) setCategory("");
    });
  };

  return (
    <div className="settings-grid">
      <Card className="settings-card settings-threshold">
        <CardHeader>
          <CardTitle>Alert window</CardTitle>
        </CardHeader>
        <CardContent>
          <p className="settings-intro">Choose how early Expiry Bud should flag an item for attention.</p>
          <div className="threshold-preview" aria-live="polite">
            <span>Alert preview</span>
            <strong>{previewDays} {previewDays === 1 ? "day" : "days"} before expiry</strong>
            <div className="threshold-scale" aria-hidden="true">
              <span className="scale-safe">Safe</span>
              <span className="scale-alert" style={{ width: `${Math.max(18, Math.min(72, 18 + previewDays * 1.2))}%` }}>Attention</span>
              <span className="scale-expired">Expired</span>
            </div>
          </div>
          <form className="inline-form" onSubmit={saveThreshold}>
            <Input
              label="Days before expiry"
              type="number"
              min="0"
              max="365"
              step="1"
              value={threshold}
              onChange={(event) => setThreshold(event.target.value)}
              required
            />
            <Button variant="primary" type="submit" loading={busy} disabled={busy}>
              Save threshold
            </Button>
          </form>
        </CardContent>
      </Card>
      <Card className="settings-card settings-categories">
        <CardHeader>
          <div className="card-title-row">
            <CardTitle>Product categories</CardTitle>
            <span>{snapshot.categories.length} total</span>
          </div>
        </CardHeader>
        <CardContent>
          <p className="settings-intro">Keep categories short and familiar so products stay easy to filter.</p>
          <form className="inline-form" onSubmit={addCategory}>
            <Input
              label="New category"
              value={category}
              onChange={(event) => setCategory(event.target.value)}
              required
            />
            <Button type="submit" variant="primary" loading={busy} disabled={busy}>
              Add category
            </Button>
          </form>
          <div className="category-list" aria-label="Saved product categories">
            {snapshot.categories.map((item, index) => (
              <div className="category-item" key={item}>
                <Badge variant="outline" className={`category-badge category-tone-${index % 4}`}>{item}</Badge>
                <Button
                  size="sm"
                  variant="ghost"
                  onClick={() => onDeleteCategory(item)}
                  disabled={busy || snapshot.categories.length === 1}
                >
                  Remove
                </Button>
              </div>
            ))}
          </div>
        </CardContent>
      </Card>
    </div>
  );
}
