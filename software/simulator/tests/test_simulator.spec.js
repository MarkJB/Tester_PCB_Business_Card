import { test, expect } from "@playwright/test";

// Locator helper that works for both grouped and ungrouped SVG buttons
const btn = (page, id) =>
  page.locator(`g#button_input_${id} rect, rect#button_input_${id}`);

let page;

test.describe("Simulator Test Cases", () => {
  test.beforeAll(async ({ browser }) => {
    const ctx = await browser.newContext();
    page = await ctx.newPage();
    await page.goto("http://localhost:5173");
    const powerSwitch = page.locator("#power_switch_rail");
    await powerSwitch.click(); // turn on power
    const powerLed = page.locator("#status_led_pwr rect");
    await expect(powerLed).toHaveAttribute("style", /fill:\s*red/i); // check power LED is on
    const readyLed = page.locator("#status_led_rdy rect");
    await expect(readyLed).toHaveAttribute("style", /fill:\s*red/i); // check ready LED is on
    const startButton = page.locator("#button_input_d");
    await startButton.click(); // start the test
  });

  // ----------------------------
  // Passing Test Cases
  // ----------------------------
  test.describe("Positive Test Cases", () => {
    test("TC1: Positive - press A,B,C within time", async () => {
      await btn(page, "a").click();
      await btn(page, "b").click();
      await btn(page, "c").click();

      await page.waitForTimeout(5100);

      const tc1PassLed = page.locator("#test_case_1_pass_led rect");
      await expect(tc1PassLed).toHaveAttribute("style", /fill:\s*lime/i);
    });

    test("TC2: Positive - double click B fast", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      await btn(page, "b").click();
      await page.waitForTimeout(50);
      await btn(page, "b").click();

      await page.waitForTimeout(5100);

      const tc2PassLed = page.locator("#test_case_2_pass_led rect");
      await expect(tc2PassLed).toHaveAttribute("style", /fill:\s*lime/i);
    });

    test("TC3: Positive - B and C pressed, A not pressed", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      await btn(page, "b").click();
      await btn(page, "c").click();

      await page.waitForTimeout(5100);

      const tc3PassLed = page.locator("#test_case_3_pass_led rect");
      await expect(tc3PassLed).toHaveAttribute("style", /fill:\s*lime/i);
    });

    test("TC4: Positive - press counts in valid ranges", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      for (let i = 0; i < 2; i++) await btn(page, "a").click();
      for (let i = 0; i < 6; i++) await btn(page, "b").click();
      for (let i = 0; i < 3; i++) await btn(page, "c").click();

      await page.waitForTimeout(5100);

      const tc4PassLed = page.locator("#test_case_4_pass_led rect");
      await expect(tc4PassLed).toHaveAttribute("style", /fill:\s*lime/i);
    });
  });

  // ----------------------------
  // Negative Test Cases
  // ----------------------------
  test.describe("Negative Test Cases", () => {
    test("TC1: Negative - miss a button", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      await btn(page, "a").click();
      await btn(page, "b").click();
      // no 'c'
      await page.waitForTimeout(5100);

      const tc1FailLed = page.locator("#test_case_1_fail_led rect");
      await expect(tc1FailLed).toHaveAttribute("style", /fill:\s*red/i);
    });

    test("TC2: Negative - second press too slow", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      await btn(page, "b").click();
      await page.waitForTimeout(500);
      await btn(page, "b").click();

      await page.waitForTimeout(5100);

      const tc2FailLed = page.locator("#test_case_2_fail_led rect");
      await expect(tc2FailLed).toHaveAttribute("style", /fill:\s*red/i);
    });

    test("TC3: Negative - A also pressed", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      await btn(page, "a").click();
      await btn(page, "b").click();
      await btn(page, "c").click();

      await page.waitForTimeout(5100);

      const tc3FailLed = page.locator("#test_case_3_fail_led rect");
      await expect(tc3FailLed).toHaveAttribute("style", /fill:\s*red/i);
    });

    test("TC4: Negative - press counts out of range", async () => {
      const startButton = page.locator("#button_input_d");
      await startButton.click(); // start the test
      for (let i = 0; i < 5; i++) await btn(page, "a").click();
      for (let i = 0; i < 1; i++) await btn(page, "b").click();
      for (let i = 0; i < 10; i++) await btn(page, "c").click();

      await page.waitForTimeout(5100);

      const tc4FailLed = page.locator("#test_case_4_fail_led rect");
      await expect(tc4FailLed).toHaveAttribute("style", /fill:\s*red/i);
    });
  });
});
