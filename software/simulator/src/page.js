// page.js - Page Object Model for simulator UI

export class Page {
  constructor() {
    // Button elements A–E
    this.buttonInputs = ["a", "b", "c", "d", "e"].reduce((acc, key) => {
      acc[key] = document.getElementById(`button_input_${key}`);
      return acc;
    }, {});

    // Status LEDs
    this.statusLEDs = ["pwr", "init", "rdy", "run", "idle"].reduce(
      (acc, key) => {
        acc[key] = this.getLEDRect(`status_led_${key}`);
        return acc;
      },
      {}
    );

    // Test Case LEDs (pass/fail)
    this.testCaseLEDs = Array.from({ length: 5 }, (_, i) => {
      const index = i + 1;
      return {
        pass: this.getLEDRect(`test_case_${index}_pass_led`),
        fail: this.getLEDRect(`test_case_${index}_fail_led`),
      };
    });

    // Power switch
    this.powerSwitch = document.getElementById("power_switch_rail");
    this.powerThumb = document.getElementById("power_switch_thumb");
    this.wrapper = document.getElementById("sim-wrapper");
  }

  getLEDRect(id) {
    const group = document.getElementById(id);
    return group?.querySelector("rect");
  }

  // Example UI actions
  clickButton(key) {
    this.buttonInputs[key]?.click();
  }

  togglePower() {
    this.powerSwitch?.click();
  }
}
