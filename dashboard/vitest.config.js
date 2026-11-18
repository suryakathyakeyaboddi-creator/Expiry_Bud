import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  test: {
    environment: "node",
    setupFiles: ["./tests/setup.js"],
    coverage: {
      include: ["dashboard/server/**/*.js", "dashboard/client/src/**/*.{js,jsx}"]
    }
  }
});
