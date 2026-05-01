import { test, expect } from "@playwright/test";

function dbName(prefix) {
  return prefix + "-" + Date.now() + "-" + Math.random().toString(36).slice(2);
}

test.beforeEach(async ({ page }) => {
  await page.goto("/");
  await expect(page.locator("#ready")).toHaveText("ready");
});

test("loads the browser runtime with RAD IndexedDB plugins", async ({
  page,
}) => {
  const plugins = await page.evaluate(function () {
    return window.browserTests.plugins();
  });

  expect(plugins.Zen).toBe(true);
  expect(plugins.Radix).toBe(true);
  expect(plugins.Radisk).toBe(true);
  expect(plugins.RindexedDB).toBe(true);
});

test("persists RAD data across page reloads in IndexedDB", async ({ page }) => {
  const file = dbName("playwright-rad");
  const soul = "browser-suite";
  const key = "hello";
  const value = "world";

  await page.evaluate(async function (name) {
    await window.browserTests.clearIndexedDB(name);
  }, file);

  await page.evaluate(
    async function (args) {
      await window.browserTests.putValue(
        "rindexed",
        args.file,
        args.soul,
        args.key,
        args.value,
      );
    },
    { file, soul, key, value },
  );

  await expect
    .poll(async () => {
      const keys = await page.evaluate(async function (name) {
        return await window.browserTests.listIndexedDBKeys(name);
      }, file);
      return keys.length;
    })
    .toBeGreaterThan(0);

  await page.reload();
  await expect(page.locator("#ready")).toHaveText("ready");

  const loaded = await page.evaluate(
    async function (args) {
      return await window.browserTests.onceValue(
        "rindexed",
        args.file,
        args.soul,
        args.key,
      );
    },
    { file, soul, key },
  );

  expect(loaded).toBe(value);
});

test("persists RAD data across page reloads in OPFS", async ({ page }) => {
  const file = dbName("playwright-opfs");
  const soul = "browser-suite";
  const key = "hello";
  const value = "world";

  const plugins = await page.evaluate(function () {
    return window.browserTests.plugins();
  });
  expect(plugins.ROPFS).toBe(true);

  await page.evaluate(async function (name) {
    await window.browserTests.clearOPFS(name);
  }, file);

  await page.evaluate(
    async function (args) {
      await window.browserTests.putValue(
        "opfs",
        args.file,
        args.soul,
        args.key,
        args.value,
      );
    },
    { file, soul, key, value },
  );

  await expect
    .poll(async () => {
      const keys = await page.evaluate(async function (name) {
        return await window.browserTests.listOPFSKeys(name);
      }, file);
      return keys.length;
    })
    .toBeGreaterThan(0);

  await page.reload();
  await expect(page.locator("#ready")).toHaveText("ready");

  const loaded = await page.evaluate(
    async function (args) {
      return await window.browserTests.onceValue(
        "opfs",
        args.file,
        args.soul,
        args.key,
      );
    },
    { file, soul, key },
  );

  expect(loaded).toBe(value);
});
