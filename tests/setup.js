import "@testing-library/jest-dom/vitest";

if (typeof window !== "undefined") {
  window.matchMedia ??= () => ({
    matches: false,
    addEventListener() {},
    removeEventListener() {},
    addListener() {},
    removeListener() {}
  });
  window.scrollTo = () => {};
  globalThis.ResizeObserver ??= class ResizeObserver {
    observe() {}
    unobserve() {}
    disconnect() {}
  };
  globalThis.SVGPathElement ??= class SVGPathElement {};
  window.SVGPathElement ??= globalThis.SVGPathElement;
}
