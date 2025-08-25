import svgContent from "../assets/elements_overlay.svg?raw";
import { Page } from "./page.js";

import {
  ledOn,
  ledOff,
  startPulseLED,
  stopPulseLED,
  flashLED,
  statusRunning,
  statusIdle,
  testCaseInProgress,
  activeFlashIntervals,
} from "./leds.js";
import { systemState, isSystemReady } from "./state.js";
import {
  powerOnSequence,
  startBootSequence,
  stopBootSequence,
} from "./boot.js";
import { testFunctions } from "./testCases.js";

let currentTestIndex = 0;

// Helper to get LED rect inside group
const getLEDRect = (id) => {
  const group = document.getElementById(id);
  return group?.querySelector("rect");
};

// Power-on sequence

document.addEventListener("DOMContentLoaded", () => {
  const wrapper = document.getElementById("sim-wrapper");
  wrapper.innerHTML = svgContent;

  // Initialize Page Object Model
  const page = new Page();

  // Status LEDs
  const statusLEDs = ["pwr", "init", "rdy", "run", "idle"].reduce(
    (acc, key) => {
      acc[key] = getLEDRect(`status_led_${key}`);
      return acc;
    },
    {}
  );

  // Assertion utilities

  // Test Case Runner
  const runTestCase = async (testFn, testLEDs, statusLEDs) => {
    // Fresh scope for this single test
    const setups = [];
    const teardowns = [];
    const fixtureAPI = {
      setup: (fn) => setups.push(fn),
      teardown: (fn) => teardowns.push(fn),
      page, // pass the page object to test cases
    };

    statusRunning(statusLEDs);
    testCaseInProgress(testLEDs.pass, testLEDs.fail, 300);

    const onVisualCue = (cue) => {
      switch (cue) {
        case "FAIL_BLINK":
          startPulseLED(testLEDs.fail, "red");
          break;
        case "STOP_BLINK":
          stopPulseLED(testLEDs.fail);
          break;
        case "PASS":
          ledOn(testLEDs.pass, "lime");
          break;
        case "FAIL":
          ledOn(testLEDs.fail, "red");
          break;
        case "OFF":
          ledOff(testLEDs.pass);
          ledOff(testLEDs.fail);
          break;
        default:
          console.warn(`Unknown visual cue: ${cue}`);
      }
    };

    try {
      // Call testFn once to register hooks + get main body promise
      const promise = testFn({ ...fixtureAPI, onVisualCue });

      // Run setups now
      for (const fn of setups) await fn();

      // Await main test logic
      const result = await promise;

      statusIdle(statusLEDs);
      onVisualCue("OFF");

      if (result.passed) {
        console.log(`✅ PASS: ${result.message}`);
        onVisualCue("PASS");
      } else {
        console.log(`❌ FAIL: ${result.message}`);
        onVisualCue("FAIL");
      }

      return result;
    } finally {
      // Run teardowns even if test fails/errors
      for (const fn of teardowns) await fn();
    }
  };

  // Test Suite runner
  const runCurrentTestCase = () => {
    if (!isSystemReady()) {
      console.log("System not ready. Cannot start tests.", systemState);
      return;
    }

    if (currentTestIndex >= testFunctions.length) {
      currentTestIndex = 0;
      resetTestCaseLEDs(testCaseLEDs);
      console.log("All test cases completed. Resetting to first test case.");
    }

    const testFn = testFunctions[currentTestIndex];
    const testLED = testCaseLEDs[currentTestIndex];
    runTestCase(testFn, testLED, statusLEDs);
    currentTestIndex++;
  };

  const handleStartTest = () => {
    if (systemState.power) {
      runCurrentTestCase();
    }
  };

  // Reset system logic
  const resetSystem = () => {
    currentTestIndex = 0;
    stopPulseLED();
    // turn off test case LEDs
    resetTestCaseLEDs(testCaseLEDs);
    // Turn off all status LEDs
    resetStatusLEDs(statusLEDs);
    // Flash power LED and restart sequence
    flashLED(statusLEDs.pwr, "red", 100, 1000);
    setTimeout(() => {
      ledOn(statusLEDs.pwr);
      powerOnSequence(statusLEDs);
    }, 1100);
  };

  // Buttons Elements A–E
  const buttonInputs = ["a", "b", "c", "d", "e"].reduce((acc, key) => {
    acc[key] = document.getElementById(`button_input_${key}`);
    return acc;
  }, {});

  // Test Case LED Elements (pass/fail)
  const testCaseLEDs = Array.from({ length: 5 }, (_, i) => {
    const index = i + 1;
    return {
      pass: getLEDRect(`test_case_${index}_pass_led`),
      fail: getLEDRect(`test_case_${index}_fail_led`),
    };
  });

  // Button handler functions
  const handleButtonA = () => {
    console.log("Input Button A pressed");
    if (systemState.power) {
      const interval = flashLED(statusLEDs.init, "red", 100, 200);
      activeFlashIntervals.push(interval);
    }
  };

  const handleButtonB = () => {
    console.log("Input Button B pressed");
    if (systemState.power) {
      const interval = flashLED(statusLEDs.init, "red", 100, 200);
      activeFlashIntervals.push(interval);
    }
  };

  const handleButtonC = () => {
    console.log("Input Button C pressed");
    if (systemState.power) {
      const interval = flashLED(statusLEDs.init, "red", 100, 200);
      activeFlashIntervals.push(interval);
    }
  };

  const handleButtonD = () => {
    console.log("'START TEST' Button pressed");
    if (systemState.power) {
      handleStartTest();
    }
  };

  const handleButtonE = () => {
    console.log("'RESET' button pressed");
    if (systemState.power) {
      resetSystem();
    }
  };

  // Map button keys to their handlers
  const buttonHandlers = {
    a: handleButtonA,
    b: handleButtonB,
    c: handleButtonC,
    d: handleButtonD,
    e: handleButtonE,
  };

  // Bind each button to its handler
  Object.entries(buttonInputs).forEach(([key, button]) => {
    button.addEventListener("click", buttonHandlers[key]);
  });

  // Power switch logic
  const togglePowerSwitch = () => {
    console.log("Toggling power switch");
    const thumb = document.getElementById("power_switch_thumb");

    if (!thumb) return;

    systemState.power = !systemState.power;

    thumb.setAttribute(
      "transform",
      systemState.power ? "matrix(1,0,0,1,0,0)" : "matrix(1,0,0,1,0,-40)"
    );

    console.log(`Power ${systemState.power ? "ON" : "OFF"}`);

    // Optional: trigger system reset or shutdown
    if (systemState.power) {
      startBootSequence(statusLEDs);
    } else {
      stopBootSequence(statusLEDs, testCaseLEDs);
    }
  };

  const powerSwitch = document.getElementById("power_switch_rail");
  if (powerSwitch) {
    powerSwitch.addEventListener("click", togglePowerSwitch);
  }
});
