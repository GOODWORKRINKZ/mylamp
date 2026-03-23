import path from "path";

import { defineConfig } from "vite";

export default defineConfig({
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