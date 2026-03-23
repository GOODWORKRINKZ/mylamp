// vite.config.ts
import path from "path";
import { defineConfig } from "vite";
var vite_config_default = defineConfig({
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
          if (path.extname(assetInfo.name ?? "") === ".css") {
            return "styles.css";
          }
          return "[name][extname]";
        }
      }
    }
  }
});
export {
  vite_config_default as default
};
//# sourceMappingURL=data:application/json;base64,ewogICJ2ZXJzaW9uIjogMywKICAic291cmNlcyI6IFsidml0ZS5jb25maWcudHMiXSwKICAic291cmNlc0NvbnRlbnQiOiBbImltcG9ydCBwYXRoIGZyb20gXCJwYXRoXCI7XG5cbmltcG9ydCB7IGRlZmluZUNvbmZpZyB9IGZyb20gXCJ2aXRlXCI7XG5cbmV4cG9ydCBkZWZhdWx0IGRlZmluZUNvbmZpZyh7XG4gIGJhc2U6IFwiLi9cIixcbiAgYnVpbGQ6IHtcbiAgICBvdXREaXI6IFwiLi4vcmVzb3VyY2VzL2Rpc3RcIixcbiAgICBlbXB0eU91dERpcjogdHJ1ZSxcbiAgICBhc3NldHNEaXI6IFwiLlwiLFxuICAgIHJvbGx1cE9wdGlvbnM6IHtcbiAgICAgIG91dHB1dDoge1xuICAgICAgICBlbnRyeUZpbGVOYW1lczogXCJzY3JpcHQuanNcIixcbiAgICAgICAgY2h1bmtGaWxlTmFtZXM6IFwiY2h1bmstW25hbWVdLmpzXCIsXG4gICAgICAgIGFzc2V0RmlsZU5hbWVzOiAoYXNzZXRJbmZvKSA9PiB7XG4gICAgICAgICAgaWYgKHBhdGguZXh0bmFtZShhc3NldEluZm8ubmFtZSA/PyBcIlwiKSA9PT0gXCIuY3NzXCIpIHtcbiAgICAgICAgICAgIHJldHVybiBcInN0eWxlcy5jc3NcIjtcbiAgICAgICAgICB9XG5cbiAgICAgICAgICByZXR1cm4gXCJbbmFtZV1bZXh0bmFtZV1cIjtcbiAgICAgICAgfVxuICAgICAgfVxuICAgIH1cbiAgfVxufSk7Il0sCiAgIm1hcHBpbmdzIjogIjtBQUFBLE9BQU8sVUFBVTtBQUVqQixTQUFTLG9CQUFvQjtBQUU3QixJQUFPLHNCQUFRLGFBQWE7QUFBQSxFQUMxQixNQUFNO0FBQUEsRUFDTixPQUFPO0FBQUEsSUFDTCxRQUFRO0FBQUEsSUFDUixhQUFhO0FBQUEsSUFDYixXQUFXO0FBQUEsSUFDWCxlQUFlO0FBQUEsTUFDYixRQUFRO0FBQUEsUUFDTixnQkFBZ0I7QUFBQSxRQUNoQixnQkFBZ0I7QUFBQSxRQUNoQixnQkFBZ0IsQ0FBQyxjQUFjO0FBQzdCLGNBQUksS0FBSyxRQUFRLFVBQVUsUUFBUSxFQUFFLE1BQU0sUUFBUTtBQUNqRCxtQkFBTztBQUFBLFVBQ1Q7QUFFQSxpQkFBTztBQUFBLFFBQ1Q7QUFBQSxNQUNGO0FBQUEsSUFDRjtBQUFBLEVBQ0Y7QUFDRixDQUFDOyIsCiAgIm5hbWVzIjogW10KfQo=
