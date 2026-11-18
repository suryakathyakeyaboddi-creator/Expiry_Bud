import { useCallback, useEffect, useMemo, useState } from "react";
import {
  Badge,
  Button,
  Card,
  CardContent,
  CardHeader,
  CardTitle,
  Input,
  NativeSelect,
  Spinner
} from "./ui.jsx";
import { Icon } from "./icons.jsx";
import { apiRequest } from "./api.js";
import { AnalyticsPanel } from "./components/AnalyticsPanel.jsx";
import { HistoryTable } from "./components/HistoryTable.jsx";
import { MetricCard } from "./components/MetricCard.jsx";
import { ProductForm } from "./components/ProductForm.jsx";
import { ProductTable } from "./components/ProductTable.jsx";
import { SettingsPanel } from "./components/SettingsPanel.jsx";

const navItems = [
  ["overview", "Overview"],
  ["products", "Products"],
  ["history", "History"],
  ["analytics", "Analytics"],
  ["settings", "Settings"]
];

export default function App() {
  const [snapshot, setSnapshot] = useState(null);
  const [view, setView] = useState("overview");
  const [editing, setEditing] = useState(null);
  const [formOpen, setFormOpen] = useState(false);
  const [search, setSearch] = useState("");
  const [status, setStatus] = useState("All");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");
  const [notice, setNotice] = useState("");

  const refresh = useCallback(async () => {
    setError("");
    const data = await apiRequest("/api/dashboard");
    setSnapshot(data);
    if (data.archivedCount) {
      setNotice(`${data.archivedCount} overdue product${data.archivedCount === 1 ? " was" : "s were"} moved to history.`);
    }
  }, []);

  useEffect(() => {
    refresh().catch((requestError) => setError(requestError.message));
  }, [refresh]);

  const runAction = async (action, successMessage) => {
    setBusy(true);
    setError("");
    try {
      await action();
      await refresh();
      setNotice(successMessage);
      return true;
    } catch (requestError) {
      setError(requestError.message);
      return false;
    } finally {
      setBusy(false);
    }
  };

  const visibleProducts = useMemo(() => {
    if (!snapshot) return [];
    const query = search.trim().toLowerCase();
    return snapshot.products.filter(
      (product) =>
        (!query ||
          product.name.toLowerCase().includes(query) ||
          product.category.toLowerCase().includes(query)) &&
        (status === "All" || product.status === status)
    );
  }, [snapshot, search, status]);

  const saveProduct = async (product) => {
    const isEditing = Boolean(editing);
    const succeeded = await runAction(
      () =>
        apiRequest(isEditing ? `/api/products/${editing.id}` : "/api/products", {
          method: isEditing ? "PUT" : "POST",
          body: JSON.stringify(product)
        }),
      isEditing ? "Product updated." : "Product added."
    );
    if (succeeded) {
      setEditing(null);
      setFormOpen(false);
    }
  };

  const markOutcome = (product, outcome) =>
    runAction(
      () =>
        apiRequest(`/api/products/${product.id}/outcome`, {
          method: "POST",
          body: JSON.stringify({ outcome })
        }),
      `${product.name} moved to history as ${outcome}.`
    );

  const deleteProduct = (product) => {
    if (!window.confirm(`Delete ${product.name} permanently?`)) return;
    runAction(
      () => apiRequest(`/api/products/${product.id}`, { method: "DELETE" }),
      `${product.name} deleted.`
    );
  };

  if (!snapshot) {
    return (
      <main className="loading-screen">
        <Spinner size="lg" />
        <p>{error || "Loading your pantry…"}</p>
        {error && <Button onClick={() => refresh().catch((requestError) => setError(requestError.message))}>Retry</Button>}
      </main>
    );
  }

  return (
    <div className="app-shell">
      <aside className="sidebar">
        <div className="brand"><span className="brand-symbol"><Icon name="leaf" /></span>Expiry Bud</div>
        <nav className="main-nav" aria-label="Dashboard sections">
          {navItems.map(([key, label]) => (
            <button key={key} className="nav-button" onClick={() => setView(key)} aria-current={view === key ? "page" : undefined}>
              <Icon name={key} />{label}
            </button>
          ))}
        </nav>
      </aside>
      <div className="workspace">
      <header className="site-header">
        <span className="workspace-name">Household pantry</span>
        <div className="header-actions">
          <Badge variant="outline">Today · {snapshot.today}</Badge>
          <Button variant="outline" onClick={() => refresh().catch((requestError) => setError(requestError.message))} disabled={busy}>
            <Icon name="refresh" /> Refresh
          </Button>
          <Button
            variant="primary"
            onClick={() => {
              setEditing(null);
              setFormOpen(true);
              setView("products");
            }}
          >
            <Icon name="plus" /> Add product
          </Button>
        </div>
      </header>

      <main className="main-content">
        {error && <div role="alert" className="notice-banner notice-error">{error}</div>}
        {notice && (
          <div role="status" className="notice-banner notice-success">
            <span>{notice}</span>
            <Button size="sm" variant="ghost" onClick={() => setNotice("")}>Dismiss</Button>
          </div>
        )}

        {view === "overview" && (
          <div className="stack">
            <section className="section-heading">
              <div>
                <p className="eyebrow">At a glance</p>
                <h2>Your household shelf today</h2>
              </div>
              <p>{snapshot.settings.alertThresholdDays}-day alert window</p>
            </section>
            <div className="metric-grid">
              <MetricCard label="Active" value={snapshot.counts.active} note="products tracked" />
              <MetricCard label="Safe" value={snapshot.counts.safe} note="outside alert window" tone="blue" />
              <MetricCard label="Expiring" value={snapshot.counts.expiring} note="needs attention" tone="orange" />
              <MetricCard label="History" value={snapshot.counts.history} note="completed records" tone="red" />
            </div>
            <div className="overview-grid">
              <Card>
                <CardHeader><CardTitle>Needs attention</CardTitle></CardHeader>
                <CardContent>
                  {!snapshot.alerts.length ? (
                    <div className="all-clear"><span>✓</span><div><strong>All clear</strong><p>No products are nearing expiry.</p></div></div>
                  ) : (
                    <div className="alert-list">
                      {snapshot.alerts.map((product) => (
                        <button className="alert-row" key={product.id} onClick={() => { setSearch(product.name); setStatus("All"); setView("products"); }}>
                          <div><strong>{product.name}</strong><span>{product.category}</span></div>
                          <Badge variant="accent">{product.daysRemaining === 0 ? "Today" : `${product.daysRemaining} days`}</Badge>
                        </button>
                      ))}
                    </div>
                  )}
                </CardContent>
              </Card>
              <Card className="waste-note">
                <CardHeader><CardTitle>Household waste rate</CardTitle></CardHeader>
                <CardContent>
                  <div className="waste-ring" aria-hidden="true" style={{ "--waste": snapshot.analytics.wasteRatePercent + "%" }} /><strong className="big-stat">{snapshot.analytics.wasteRatePercent.toFixed(1)}%</strong>
                  <p>of completed items became waste.</p>
                  <div className="note-rule" />
                  <p><strong>Estimated loss:</strong> Rs. {snapshot.analytics.estimatedLoss.toFixed(2)}</p>
                  <Button variant="outline" onClick={() => setView("analytics")}>See analytics</Button>
                </CardContent>
              </Card>
            </div>
          </div>
        )}

        {view === "products" && (
          <div className="stack">
            <section className="section-heading"><div><p className="eyebrow">Active inventory</p><h2>Products</h2></div><span>{visibleProducts.length} shown</span></section>
            {formOpen && (
              <ProductForm
                categories={snapshot.categories}
                product={editing}
                onSubmit={saveProduct}
                onCancel={() => { setFormOpen(false); setEditing(null); }}
                busy={busy}
              />
            )}
            <div className="product-toolbar">
              <Input label="Search" placeholder="Name or category" value={search} onChange={(event) => setSearch(event.target.value)} />
              <label className="field-label">Status<NativeSelect aria-label="Status" value={status} onChange={(event) => setStatus(event.target.value)} options={["All", "Safe", "Expiring", "Expired"].map((item) => ({ value: item, label: item }))} /></label>
            </div>
            <ProductTable
              products={visibleProducts}
              onEdit={(product) => { setEditing(product); setFormOpen(true); window.scrollTo({ top: 0, behavior: "smooth" }); }}
              onOutcome={markOutcome}
              onDelete={deleteProduct}
              busy={busy}
            />
          </div>
        )}

        {view === "history" && <div className="stack"><section className="section-heading"><div><p className="eyebrow">Lifecycle archive</p><h2>Product history</h2></div><span>{snapshot.history.length} records</span></section><HistoryTable history={snapshot.history} /></div>}
        {view === "analytics" && <div className="stack"><section className="section-heading section-heading-analytics"><div><p className="eyebrow">Patterns over time</p><h2>Waste analytics</h2></div><span>{snapshot.analytics.mostWastedCategory ? `Most wasted: ${snapshot.analytics.mostWastedCategory}` : "No waste recorded"}</span></section><AnalyticsPanel analytics={snapshot.analytics} /></div>}
        {view === "settings" && (
          <div className="stack">
            <section className="section-heading section-heading-settings"><div><p className="eyebrow">Household preferences</p><h2>Settings</h2></div><span>Changes save to your local files</span></section>
            <SettingsPanel
              snapshot={snapshot}
              busy={busy}
              onThreshold={(value) => runAction(() => apiRequest("/api/settings", { method: "PUT", body: JSON.stringify({ alertThresholdDays: value }) }), "Alert threshold updated.")}
              onAddCategory={(name) => runAction(() => apiRequest("/api/categories", { method: "POST", body: JSON.stringify({ name }) }), "Category added.")}
              onDeleteCategory={(name) => runAction(() => apiRequest(`/api/categories/${encodeURIComponent(name)}`, { method: "DELETE" }), "Category removed.")}
            />
          </div>
        )}
      </main>
      </div>
    </div>
  );
}
