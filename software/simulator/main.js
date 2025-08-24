import svgContent from "./assets/elements_overlay.svg?raw";

let currentTestIndex = 0;

const systemState = {
  power: false,
  init: false,
  ready: false,
  idle: false,
  running: false,
};

const ledOn = (element, colour = "red") => {
  element.style.fill = colour;
  element.style.filter = `drop-shadow(0 0 6px ${colour})`;
  element.style.transition = "fill 0.1s ease-in-out, filter 0.1s ease-in-out";
};

const ledOff = (element) => {
  element.style.fill = "white";
  element.style.filter = "none";
  element.style.transition = "fill 0.1s ease-in-out, filter 0.1s ease-in-out";
};

const isLEDOn = (element, expectedColour = "red") => {
  return element?.style.fill === expectedColour;
};

const startPulseLED = (ledElement, interval, colour = "red") => {
  let isOn = false;

  const pulse = setInterval(() => {
    if (!systemState.power) {
      clearInterval(pulse);
      ledOff(ledElement);
      return;
    }

    isOn ? ledOff(ledElement) : ledOn(ledElement, colour);
    isOn = !isOn;
  }, interval);

  activeFlashIntervals.push(pulse);
};

const stopPulseLED = () => {
  activeFlashIntervals.forEach(clearInterval);
  activeFlashIntervals = [];
};

const isSystemReady = () => {
  return systemState.ready && systemState.idle && !systemState.running;
};

const statusRunning = (statusLEDs) => {
  stopPulseLED(); // clears any previous intervals
  ledOff(statusLEDs.idle); // ensure idle LED is off
  startPulseLED(statusLEDs.run, 500); // fast pulse for active state

  systemState.idle = false;
  systemState.running = true;
};

const statusIdle = (statusLEDs) => {
  stopPulseLED();
  ledOff(statusLEDs.run);
  startPulseLED(statusLEDs.idle, 1500); // slow pulse for idle state

  systemState.idle = true;
  systemState.running = false;
};

const flashLED = (
  element,
  colour = "lime",
  frequency = 100,
  duration = 500
) => {
  let elapsed = 0;
  const interval = setInterval(() => {
    if (!systemState.power) {
      clearInterval(interval);
      ledOff(element);
      return;
    }

    element.style.fill === "white" ? ledOn(element, colour) : ledOff(element);
    elapsed += frequency;
    if (elapsed >= duration) {
      clearInterval(interval);
      ledOff(element);
    }
  }, frequency);
  return interval;
};

const testCaseInProgress = (passLED, failLED, interval = 500) => {
  let toggle = false;

  const flash = setInterval(() => {
    if (!systemState.running) {
      clearInterval(flash);
      ledOff(passLED);
      ledOff(failLED);
      return;
    }

    if (toggle) {
      ledOn(passLED, "lime");
      ledOff(failLED);
    } else {
      ledOn(failLED, "red");
      ledOff(passLED);
    }

    toggle = !toggle;
  }, interval);

  activeFlashIntervals.push(flash);
};

// Helper to get LED rect inside group
const getLEDRect = (id) => {
  const group = document.getElementById(id);
  return group?.querySelector("rect");
};

// Power-on sequence
const powerOnSequence = (statusLEDs) => {
  const { pwr, init, rdy, run, idle } = statusLEDs;

  ledOn(pwr);
  setTimeout(() => {
    const interval = flashLED(init, "red", 200, 2000);
    activeFlashIntervals.push(interval);

    setTimeout(() => {
      ledOn(rdy);
      ledOff(run);
      startPulseLED(idle, 1500);
      systemState.ready = true;
      systemState.idle = true;
      systemState.running = false;
    }, 2000);
  }, 500);
};

// Interuptable power-on sequence

let bootSequenceTimeouts = [];
let activeFlashIntervals = [];

const startBootSequence = (statusLEDs) => {
  if (!systemState.power) return;
  currentTestIndex = 0;

  const { pwr, init, rdy, run, idle } = statusLEDs;

  // Step 1: Power LED on immediately
  ledOn(pwr);

  // Step 2: Flash init for 2 seconds
  systemState.init = true;
  const initFlashDuration = 2000;
  const interval = flashLED(init, "red", 200, initFlashDuration);
  activeFlashIntervals.push(interval);

  // Step 3: After init flash completes, light up rdy and idle
  const timeout = setTimeout(() => {
    if (!systemState.power) return;
    systemState.init = false;
    systemState.ready = true;
    systemState.idle = true;
    ledOn(rdy);
    statusIdle(statusLEDs);
  }, initFlashDuration);

  bootSequenceTimeouts.push(timeout);
};

const stopBootSequence = (statusLEDs, testCaseLEDs) => {
  bootSequenceTimeouts.forEach(clearTimeout);
  bootSequenceTimeouts = [];

  activeFlashIntervals.forEach(clearInterval);
  activeFlashIntervals = [];

  resetStatusLEDs(statusLEDs);
  resetTestCaseLEDs(testCaseLEDs);
  currentTestIndex = 0;
};

const resetTestCaseLEDs = (testCaseLEDs) => {
  testCaseLEDs.forEach(({ pass, fail }) => {
    ledOff(pass);
    ledOff(fail);
  });
};

const resetStatusLEDs = (statusLEDs) => {
  Object.values(statusLEDs).forEach(ledOff);
};

document.addEventListener("DOMContentLoaded", () => {
  const wrapper = document.getElementById("sim-wrapper");
  wrapper.innerHTML = svgContent;

  // Status LEDs
  const statusLEDs = ["pwr", "init", "rdy", "run", "idle"].reduce(
    (acc, key) => {
      acc[key] = getLEDRect(`status_led_${key}`);
      return acc;
    },
    {}
  );

  // Test case assertion logic
  const createTestResult = (passed, message = "", details = {}) => ({
    passed,
    message,
    details,
  });

  const assert = (condition, message) => {
    if (!condition) throw new Error(message);
  };

  const assertResult = (condition, message, details = {}) => {
    try {
      assert(condition, message);
      return createTestResult(true, message, details);
    } catch (err) {
      return createTestResult(false, err.message, details);
    }
  };

  // Test Cases
  const testCase1 = () => {
    return new Promise((resolve) => {
      const requiredButtons = ["a", "b", "c"];
      const pressedButtons = new Set();

      const listeners = requiredButtons.map((key) => {
        const handler = () => pressedButtons.add(key);
        buttonInputs[key].addEventListener("click", handler);
        return { key, handler };
      });

      setTimeout(() => {
        listeners.forEach(({ key, handler }) =>
          buttonInputs[key].removeEventListener("click", handler)
        );

        const allPressed = requiredButtons.every((key) =>
          pressedButtons.has(key)
        );

        resolve(
          assertResult(allPressed, "All required buttons pressed", {
            pressedButtons,
          })
        );
      }, 5000);
    });
  };

  const testCase2 = () => {
    return new Promise((resolve) => {
      const pressTimes = [];
      const FAST_THRESHOLD = 200;

      const handler = () => {
        const now = Date.now();
        pressTimes.push(now);
        console.log(`Button B pressed at ${now}`);
      };

      buttonInputs.b.addEventListener("click", handler);

      setTimeout(() => {
        buttonInputs.b.removeEventListener("click", handler);

        const intervals =
          pressTimes.length > 1
            ? pressTimes.slice(1).map((t, i) => t - pressTimes[i])
            : [];

        const enoughPresses = pressTimes.length >= 2;
        const allFast = intervals.every((delay) => delay < FAST_THRESHOLD);

        const conditionMet = enoughPresses && allFast;

        let message = undefined;
        if (!conditionMet) {
          if (!enoughPresses) {
            message = "Not enough presses";
          } else if (!allFast) {
            message = "One or more intervals too slow";
          }
        }

        const result = assertResult(conditionMet, message, {
          pressCount: pressTimes.length,
          intervals,
        });

        resolve(result);
      }, 5000);
    });
  };

  const testCase3 = () => {
    return new Promise((resolve) => {
      const { pass, fail } = testCaseLEDs[2];
      statusRunning(statusLEDs);
      testCaseInProgress(pass, fail, 300);

      const inputState = { a: 0, b: 0, c: 0 };
      const requiredButtons = ["a", "b", "c"];
      const inputWindow = 5000;

      const handleInput = (key) => {
        if (!systemState.ready) return;
        inputState[key] = 1;
      };

      const listeners = requiredButtons.map((key) => {
        const el = buttonInputs[key];
        const handler = () => handleInput(key);
        el?.addEventListener("click", handler);
        return { el, handler };
      });

      setTimeout(() => {
        listeners.forEach(({ el, handler }) => {
          el?.removeEventListener("click", handler);
        });
        statusIdle(statusLEDs);

        const isPass =
          inputState.a === 0 && inputState.b === 1 && inputState.c === 1;

        resolve(
          assertResult(isPass, "Incorrect input pattern", {
            inputState,
          })
        );
      }, inputWindow);
    });
  };

  // Test cases to run (in sequence)
  const testFunctions = [testCase1, testCase2, testCase3];

  // Test Case Runner
  const runTestCase = async (testFn, testLEDs, statusLEDs) => {
    statusRunning(statusLEDs);
    testCaseInProgress(testLEDs.pass, testLEDs.fail, 300);

    const result = await testFn();

    statusIdle(statusLEDs);
    ledOff(testLEDs.pass);
    ledOff(testLEDs.fail);

    if (result.passed) {
      console.log(`✅ PASS: ${result.message}`);
      ledOn(testLEDs.pass, "lime");
    } else {
      console.log(`❌ FAIL: ${result.message}`);
      ledOn(testLEDs.fail, "red");
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
