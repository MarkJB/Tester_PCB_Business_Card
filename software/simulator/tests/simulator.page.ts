import { Page, expect, Locator } from "@playwright/test";

export class SimulatorPage {
  async reset() {
    await this.button("e").click();
  }
  powerSwitch: Locator;

  constructor(public page: Page) {
    this.powerSwitch = this.page.locator("#power_switch_rail");
  }

  // --- Locators ---
  button(id: string) {
    return this.page.locator(
      `g#button_input_${id} rect, rect#button_input_${id}`
    );
  }
  led(id: string) {
    return this.page.locator(`#${id} rect`);
  }

  // --- Actions ---
  async togglePowerSwitch() {
    await this.powerSwitch.click();
  }

  async powerOn() {
    await this.powerSwitch.click();
    await this.expectLedColor("status_led_pwr", "red");
    await this.expectLedColor("status_led_rdy", "red");
  }

  async powerOff() {
    await this.powerSwitch.click();
    await this.expectLedColor("status_led_pwr", "white");
    await this.expectLedColor("status_led_rdy", "white");
  }

  async startTest() {
    await this.button("d").click();
  }

  async clickButton(id: string, count = 1, delayMs = 0) {
    for (let i = 0; i < count; i++) {
      await this.button(id).click();
      if (delayMs && i < count - 1) {
        await this.page.waitForTimeout(delayMs);
      }
    }
  }

  /**
   * Sample an LED's color multiple times to check for flashing.
   * Returns a Set of all colors seen during the sample window.
   */
  async sampleLedColors(
    ledId: string,
    sampleCount = 5,
    intervalMs = 100
  ): Promise<Set<string>> {
    const colors = new Set<string>();
    for (let i = 0; i < sampleCount; i++) {
      const style = await this.led(ledId).getAttribute("style");
      const match = style?.match(/fill:\s*([^;]+)/i);
      if (match) colors.add(match[1].trim());
      console.log(`Sampled LED ${ledId}: ${match ? match[1] : "unknown"}`);
      console.log(`Sample ${i + 1}/${sampleCount}`);
      console.log(`Colors so far: ${Array.from(colors).join(", ")}`);
      await this.page.waitForTimeout(intervalMs);
    }
    return colors;
  }

  /**
   * Assert that an LED flashes the given color within a sample window.
   */

  async expectLedFlashing(
    ledId: string,
    color: string,
    sampleCount = 5,
    intervalMs = 100
  ) {
    const colors = await this.sampleLedColors(ledId, sampleCount, intervalMs);
    expect(colors.has(color)).toBeTruthy();
  }

  /**
   * Assert that pass/fail LEDs flashes between two colors within a sample window.
   */
  async expectLedsFlashing(
    passLEDId: string,
    failLEDId: string,
    passColour = "lime",
    failColour = "red",
    sampleCount = 5,
    intervalMs = 100
  ) {
    // Sample the pass and fail LEDs at the same time using promise.all
    const [passLEDColors, failLEDColors] = await Promise.all([
      this.sampleLedColors(passLEDId, sampleCount, intervalMs),
      this.sampleLedColors(failLEDId, sampleCount, intervalMs),
    ]);
    console.log("Pass LED colors:", passLEDColors);
    console.log("Fail LED colors:", failLEDColors);

    expect(passLEDColors.has(passColour)).toBeTruthy();
    expect(failLEDColors.has(failColour)).toBeTruthy();
  }

  /**
   * Assert that all test case LEDs are off (white).
   */
  async expectAllTestCaseLedsOff(caseCount = 5) {
    for (let i = 1; i <= caseCount; i++) {
      await this.expectLedColor(`test_case_${i}_pass_led`, "white");
      await this.expectLedColor(`test_case_${i}_fail_led`, "white");
    }
  }

  async expectLedColor(ledId: string, color: "lime" | "red" | "white") {
    console.log(`Expect LED ${ledId} to be ${color}`);
    await expect(this.led(ledId)).toHaveAttribute(
      "style",
      new RegExp(`fill:\\s*${color}`, "i")
    );
  }

  async waitWindow(ms = 5100) {
    await this.page.waitForTimeout(ms);
  }
}
