import path from "path";

import { defineConfig } from "vite";
import { mockApiPlugin } from "./mockApi.mjs";

export default defineConfig({
  plugins: [mockApiPlugin()],
  base: "./",
  build: {
    outDir: "../resources/dist",
    emptyOutDir: true,
    assetsDir: ".",
    rollupOptions: {
      output: {
        entryFileNames: "script.js",
        chunkFileNames: "chunk-[name].js",
        assetFileNames: (assetInfo) => {
          const assetName = assetInfo.name ? assetInfo.name : "";

          if (path.extname(assetName) === ".css") {
            return "styles.css";
          }

          return "[name][extname]";
        }
      }
    }
  }
});