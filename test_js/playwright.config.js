import { defineConfig } from '@playwright/test';

export default defineConfig({
  testDir: './test',
  testMatch: /^(?!.*[\\/](config|helpers)\.js$).*\.js$/,
  fullyParallel: false,
  forbidOnly: !!process.env.CI,
  retries: 0,
  workers: 1,
  reporter: [
    ['list'],
    ['html', { outputFolder: 'playwright-report', open: 'never' }],
  ],
  timeout: 60000,
  expect: { timeout: 10000 },
  use: {
    baseURL: process.env.DOODLE_BASE_URL || 'http://localhost:50025',
    ignoreHTTPSErrors: true,
    trace: 'retain-on-failure',
  },
});
