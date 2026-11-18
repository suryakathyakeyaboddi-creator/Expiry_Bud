import path from "node:path";
import { fileURLToPath } from "node:url";
import { createApp } from "./app.js";

const serverDirectory = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(serverDirectory, "../..");
const dataDirectory = process.env.EXPIRYBUD_DATA_DIR || path.join(projectRoot, "data");
const staticDirectory = path.join(projectRoot, "dist");
const port = Number(process.env.PORT || 4174);

const app = createApp({ dataDirectory, staticDirectory });
app.listen(port, "127.0.0.1", () => {
  console.log(`Expiry Bud dashboard: http://127.0.0.1:${port}`);
});
