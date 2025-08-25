import { defineConfig } from "@playwright/test";

export default defineConfig({
  testDir: "./tests",
  use: {
    baseURL: process.env.CI ? "http://localhost:4173" : "http://localhost:5173",
  },
  reporter: [["html", { outputFolder: "test-results" }]],
});
