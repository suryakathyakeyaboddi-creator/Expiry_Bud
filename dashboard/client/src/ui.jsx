import { useId } from "react";

export function Button({ variant = "outline", size, loading, disabled, type = "button", className = "", ...props }) {
  const styles = { primary: "btn-primary", outline: "btn-outline-secondary", ghost: "btn-light" };
  return <button {...props} type={type} disabled={disabled || loading} aria-busy={loading || undefined}
    className={["btn", styles[variant], size === "sm" && "btn-sm", className].filter(Boolean).join(" ")} />;
}

export function Badge({ className = "", variant, ...props }) {
  return <span className={`tag ${variant === "accent" ? "tag-alert" : ""} ${className}`} {...props} />;
}

export function Input({ label, id, ...props }) {
  const generated = useId();
  const inputId = id ?? generated;
  return <div className="field"><label htmlFor={inputId}>{label}</label><input {...props} id={inputId} className="form-control" /></div>;
}

export function NativeSelect({ options, ...props }) {
  return <select className="form-select" {...props}>{options.map(({ value, label }) => <option key={value} value={value}>{label}</option>)}</select>;
}

export function Card({ className = "", ...props }) {
  return <section className={`card panel ${className}`} {...props} />;
}
export function CardHeader(props) { return <header className="panel-header" {...props} />; }
export function CardContent(props) { return <div className="panel-content" {...props} />; }
export function CardTitle(props) { return <h3 {...props} />; }
export function Empty(props) { return <section className="empty-panel" {...props} />; }
export function Spinner() { return <span className="spinner-border text-success" aria-hidden="true" />; }
