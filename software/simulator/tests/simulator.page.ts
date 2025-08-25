import { Page, expect, Locator } from "@playwright/test";

export class SimulatorPage {
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
  async powerOn() {
    await this.powerSwitch.click();
    await this.expectLedColor("status_led_pwr", "red");
    await this.expectLedColor("status_led_rdy", "red");
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

  async expectLedColor(ledId: string, color: "lime" | "red") {
    await expect(this.led(ledId)).toHaveAttribute(
      "style",
      new RegExp(`fill:\\s*${color}`, "i")
    );
  }

  async waitWindow(ms = 5100) {
    await this.page.waitForTimeout(ms);
  }
}
