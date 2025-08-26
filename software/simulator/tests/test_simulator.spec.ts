import { expect, test } from "@playwright/test";
import { SimulatorPage } from "./simulator.page";

let sim: SimulatorPage;

test.describe("Simulator Staus LED Checks", () => {
  test.beforeEach(async ({ page }) => {
    sim = new SimulatorPage(page);
    await page.goto("/");
  });

  test.describe("Status LED Checks", () => {
    test("Status LEDs: Power On Sequence", async () => {
      await sim.togglePowerSwitch();
      // Check initial LED states
      await sim.expectLedColor("status_led_pwr", "red");
      await sim.expectLedColor("status_led_rdy", "white");
      await sim.expectLedColor("status_led_run", "white");
      await sim.expectLedColor("status_led_idle", "white");
      // INIT should flash red/white before RDY is on
      await sim.expectLedFlashing("status_led_init", "red", 5, 100);
      // When INIT is done, RDY should be solid red
      await sim.expectLedColor("status_led_rdy", "red");
      // IDLE should flash red/white
      await sim.expectLedFlashing("status_led_idle", "red", 5, 300);
      // Everything else should be off
      await sim.expectLedColor("status_led_run", "white");
      await sim.expectLedColor("status_led_init", "white");
    });

    test("Status LEDs: Power Off", async () => {
      await sim.powerOn(); // Note: powerOn waits for PWR and RDY to be red

      await sim.powerOff(); // Note: powerOff waits for PWR and RDY to be white

      await sim.expectLedColor("status_led_init", "white");
      await sim.expectLedColor("status_led_run", "white");
      await sim.expectLedColor("status_led_idle", "white");
    });

    test("Status LEDs: Reset clears in progress test case LEDs", async () => {
      await sim.powerOn();
      await sim.startTest();
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.expectLedColor("status_led_idle", "white");

      // Check that the pass/fail LEDs are flashing alternately red/green
      await sim.expectLedsFlashing(
        "test_case_1_pass_led",
        "test_case_1_fail_led"
      );
      await sim.reset();
      // All test case LEDs should be off
      await sim.expectAllTestCaseLedsOff();
    });

    test("Status LEDs: Reset clears previous test result", async () => {
      await sim.powerOn();
      await sim.startTest();

      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.expectLedColor("status_led_idle", "white");

      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.clickButton("c");

      await sim.expectLedFlashing("status_led_idle", "red", 11, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_1_pass_led", "lime");

      await sim.startTest();
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.expectLedColor("status_led_idle", "white");

      // Check that the pass/fail LEDs for the next test are flashing alternately red/green
      await sim.expectLedsFlashing(
        "test_case_2_pass_led",
        "test_case_2_fail_led"
      );
      await sim.reset();
      // All test case LEDs should be off
      await sim.expectAllTestCaseLedsOff();
    });
  });
});

test.describe("Simulator Test Case tests", () => {
  test.beforeAll(async ({ browser }) => {
    // Because these tests run in sequence on the simulator, we need to
    // keep the same page and context for all of them, so the simulator
    // state is preserved between tests.
    const ctx = await browser.newContext();
    const page = await ctx.newPage();
    sim = new SimulatorPage(page);

    await page.goto("/");
    await sim.powerOn();
  });

  test.describe("Positive Cases", () => {
    test("TC1: press A,B,C within time", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.clickButton("c");

      // Wait for the test to complete by checking the status of the IDLE
      // led long enough to be sure the test is done - this is a bit
      // of a kludge but works for now. We should really have a better way
      // to detect that the test is complete. Probably do something like
      // checking both simultaneously until we see the expected states.
      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_1_pass_led", "lime");
    });

    test("TC2: double click B fast", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("b");
      await sim.clickButton("b", 1, 50);

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_2_pass_led", "lime");
    });

    test("TC3: B & C pressed, A not pressed", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("b");
      await sim.clickButton("c");

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_3_pass_led", "lime");
    });

    test("TC4: counts in valid ranges", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("a", 2);
      await sim.clickButton("b", 6);
      await sim.clickButton("c", 3);

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_4_pass_led", "lime");
    });
  });

  test.describe("Negative Cases", () => {
    test("TC1: miss a button", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("a");
      await sim.clickButton("b");

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_1_fail_led", "red");
    });

    test("TC2: second press too slow", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("b");
      await sim.waitWindow(500);
      await sim.clickButton("b");

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_2_fail_led", "red");
    });

    test("TC3: A also pressed", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("a");
      await sim.clickButton("b");
      await sim.clickButton("c");

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_3_fail_led", "red");
    });

    test("TC4: counts out of range", async () => {
      await sim.startTest();
      await sim.expectLedFlashing("status_led_idle", "white", 5, 300);
      await sim.expectLedFlashing("status_led_run", "red", 5, 300);
      await sim.clickButton("a", 5);
      await sim.clickButton("b", 1);
      await sim.clickButton("c", 10);

      await sim.expectLedFlashing("status_led_idle", "red", 10, 500);
      await sim.expectLedFlashing("status_led_run", "white", 5, 300);

      await sim.expectLedColor("test_case_4_fail_led", "red");
    });
  });
});
