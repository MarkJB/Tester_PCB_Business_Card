import { test } from "@playwright/test";
import { SimulatorPage } from "./simulator.page";

let sim: SimulatorPage;

test.describe("Simulator Test Cases", () => {
  test.beforeAll(async ({ browser }) => {
    const ctx = await browser.newContext();
    const page = await ctx.newPage();
    sim = new SimulatorPage(page);

    await page.goto("http://localhost:5173");
    await sim.powerOn();
  });

  test.describe("Positive Cases", () => {
    test("TC1: press A,B,C within time", async () => {
      await sim.startTest();
      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.clickButton("c");
      await sim.waitWindow();
      await sim.expectLedColor("test_case_1_pass_led", "lime");
    });

    test("TC2: double click B fast", async () => {
      await sim.startTest();
      await sim.clickButton("b");
      await sim.clickButton("b", 1, 50);
      await sim.waitWindow();
      await sim.expectLedColor("test_case_2_pass_led", "lime");
    });

    test("TC3: B & C pressed, A not pressed", async () => {
      await sim.startTest();
      await sim.clickButton("b");
      await sim.clickButton("c");
      await sim.waitWindow();
      await sim.expectLedColor("test_case_3_pass_led", "lime");
    });

    test("TC4: counts in valid ranges", async () => {
      await sim.startTest();
      await sim.clickButton("a", 2);
      await sim.clickButton("b", 6);
      await sim.clickButton("c", 3);
      await sim.waitWindow();
      await sim.expectLedColor("test_case_4_pass_led", "lime");
    });
  });

  test.describe("Negative Cases", () => {
    test("TC1: miss a button", async () => {
      await sim.startTest();
      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.waitWindow();
      await sim.expectLedColor("test_case_1_fail_led", "red");
    });

    test("TC2: second press too slow", async () => {
      await sim.startTest();
      await sim.clickButton("b");
      await sim.waitWindow(500);
      await sim.clickButton("b");
      await sim.waitWindow();
      await sim.expectLedColor("test_case_2_fail_led", "red");
    });

    test("TC3: A also pressed", async () => {
      await sim.startTest();
      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.clickButton("c");
      await sim.waitWindow();
      await sim.expectLedColor("test_case_3_fail_led", "red");
    });

    test("TC4: counts out of range", async () => {
      await sim.startTest();
      await sim.clickButton("a", 5);
      await sim.clickButton("b", 1);
      await sim.clickButton("c", 10);
      await sim.waitWindow();
      await sim.expectLedColor("test_case_4_fail_led", "red");
    });
  });
});
