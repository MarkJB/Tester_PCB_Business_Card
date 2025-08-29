import { assertResult } from "./utils.js";
import { systemState } from "./state.js";
import { clearTestFlashIntervals } from "./leds.js";

// All test cases now accept a `page` argument for DOM access
export const testCase1 = ({ setup, teardown, page }) => {
  const requiredButtons = ["a", "b", "c"];
  const pressedButtons = new Set();
  const listeners = [];
  setup(async () => {
    requiredButtons.forEach((key) => {
      const handler = () => pressedButtons.add(key);
      page.buttonInputs[key].addEventListener("click", handler);
      listeners.push({ key, handler });
    });
    console.log("🔧 TestCase1 setup: listeners attached");
  });
  teardown(async () => {
    listeners.forEach(({ key, handler }) =>
      page.buttonInputs[key].removeEventListener("click", handler)
    );
    console.log("🧹 TestCase1 teardown: listeners removed");
  });
  return new Promise((resolve) => {
    setTimeout(() => {
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

export const testCase2 = ({ setup, teardown, page }) => {
  const pressTimes = [];
  const FAST_THRESHOLD = 200;
  let handler;
  setup(async () => {
    handler = () => {
      const now = Date.now();
      pressTimes.push(now);
      console.log(`Button B pressed at ${now}`);
    };
    page.buttonInputs.b.addEventListener("click", handler);
    console.log("🔧 TestCase2 setup: listener for Button B attached");
  });
  teardown(async () => {
    if (handler) {
      page.buttonInputs.b.removeEventListener("click", handler);
      console.log("🧹 TestCase2 teardown: listener for Button B removed");
    }
  });
  return new Promise((resolve) => {
    setTimeout(() => {
      const intervals =
        pressTimes.length > 1
          ? pressTimes.slice(1).map((t, i) => t - pressTimes[i])
          : [];
      const enoughPresses = pressTimes.length >= 2;
      const allFast = intervals.every((delay) => delay < FAST_THRESHOLD);
      const conditionMet = enoughPresses && allFast;
      let message;
      if (!conditionMet) {
        if (!enoughPresses) {
          message = "Not enough presses";
        } else if (!allFast) {
          message = "One or more intervals too slow";
        }
      }
      resolve(
        assertResult(conditionMet, message, {
          pressCount: pressTimes.length,
          intervals,
        })
      );
    }, 5000);
  });
};

export const testCase3 = ({ setup, teardown, page }) => {
  const inputState = { a: 0, b: 0, c: 0 };
  const requiredButtons = ["a", "b", "c"];
  const inputWindow = 5000;
  const listeners = [];
  setup(async () => {
    requiredButtons.forEach((key) => {
      const el = page.buttonInputs[key];
      const handler = () => {
        if (!systemState?.ready) return;
        inputState[key] = 1;
      };
      el?.addEventListener("click", handler);
      listeners.push({ el, handler });
    });
    console.log("🔧 TestCase3 setup: listeners attached for A, B, C");
  });
  teardown(async () => {
    listeners.forEach(({ el, handler }) => {
      el?.removeEventListener("click", handler);
    });
    console.log("🧹 TestCase3 teardown: listeners removed");
  });
  return new Promise((resolve) => {
    setTimeout(() => {
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

export const testCase4 = ({ setup, teardown, page }) => {
  const inputWindow = 5000;
  const pressCounts = { a: 0, b: 0, c: 0 };
  const listeners = [];
  const validRanges = {
    a: (count) => count >= 1 && count <= 3,
    b: (count) => count >= 5 && count <= 7,
    c: (count) => count >= 2 && count <= 4,
  };
  const handlePress = (key) => {
    if (!systemState?.ready) return;
    pressCounts[key] += 1;
  };
  setup(async () => {
    ["a", "b", "c"].forEach((key) => {
      const el = page.buttonInputs[key];
      const handler = () => handlePress(key);
      el?.addEventListener("click", handler);
      listeners.push({ el, handler });
    });
    console.log("🔧 TestCase4 setup: listeners attached for A, B, C");
  });
  teardown(async () => {
    listeners.forEach(({ el, handler }) => {
      el?.removeEventListener("click", handler);
    });
    console.log("🧹 TestCase4 teardown: listeners removed");
  });
  return new Promise((resolve) => {
    setTimeout(() => {
      const results = {};
      let anyValid = false;
      ["a", "b", "c"].forEach((key) => {
        const count = pressCounts[key];
        const isValid = validRanges[key](count);
        results[key] = { count, isValid };
        if (isValid) {
          anyValid = true;
        }
      });
      resolve(
        assertResult(anyValid, "No valid inputs", {
          pressCounts,
          results,
        })
      );
    }, inputWindow);
  });
};

export const testCase5 = ({ setup, teardown, page, onVisualCue }) => {
  const correctSequence = ["a", "b", "c", "b", "a"];
  const MAX_INPUTS = 5;
  const MAX_ATTEMPTS = 3;
  const RECOVERY_WINDOW_MS = 5000;

  let inputSequence = [];
  let recoveryBuffer = [];
  let inRecovery = false;
  let lastWasC = false;
  let attempts = 0;
  let recoveryStartTime = null;
  let resolved = false;
  const listeners = [];
  let resolve;

  const resolveTest = (pass, message, data = {}) => {
    if (resolved) return;
    resolved = true;
    teardownListeners();
    resolve(assertResult(pass, message, data));
  };

  const teardownListeners = () => {
    listeners.forEach(({ el, handler }) =>
      el?.removeEventListener("click", handler)
    );
    console.log("🧹 TestCase5 teardown: listeners removed");
  };

  const enterRecovery = () => {
    inRecovery = true;
    recoveryBuffer = [];
    lastWasC = false;
    recoveryStartTime = Date.now();
    clearTestFlashIntervals();
    onVisualCue?.("OFF");
    onVisualCue?.("FAIL_BLINK");
    console.log("🔁 Entered RECOVERY state");
  };

  const recoverAndReset = () => {
    inRecovery = false;
    recoveryBuffer = [];
    lastWasC = false;
    recoveryStartTime = null;
    inputSequence = [];
    onVisualCue?.("STOP_BLINK");
    onVisualCue?.("TEST_IN_PROGRESS");
    console.log("🔄 Recovery successful → test reset");
  };

  const handleInput = (key) => {
    if (!systemState?.ready || resolved) return;

    if (inRecovery) {
      recoveryBuffer.push(key);
      if (key === "c") {
        if (lastWasC) {
          recoverAndReset();
        } else {
          lastWasC = true;
        }
      } else {
        lastWasC = false;
      }
      return;
    }

    if (inputSequence.length === 0) {
      onVisualCue?.("TEST_IN_PROGRESS");
    }

    if (inputSequence.length < MAX_INPUTS) {
      inputSequence.push(key);

      if (inputSequence.length === MAX_INPUTS) {
        const isCorrect = inputSequence.every(
          (val, idx) => val === correctSequence[idx]
        );
        if (isCorrect) {
          resolveTest(true, "Unlock sequence correct", {
            attempts,
            inputSequence,
          });
        } else {
          attempts += 1;
          if (attempts >= MAX_ATTEMPTS) {
            resolveTest(false, "Maximum attempts reached", {
              attempts,
              inputSequence,
            });
          } else {
            enterRecovery();
          }
        }
      }
    }
  };

  setup(async () => {
    ["a", "b", "c"].forEach((key) => {
      const el = page.buttonInputs[key];
      const handler = () => handleInput(key);
      el?.addEventListener("click", handler);
      listeners.push({ el, handler });
    });
    console.log("🔧 TestCase5 setup: listeners attached for A, B, C");
  });

  teardown(async () => {
    teardownListeners();
  });

  return new Promise((res) => {
    resolve = res;

    const tick = () => {
      if (resolved) return;

      const now = Date.now();

      if (inRecovery && now - recoveryStartTime > RECOVERY_WINDOW_MS) {
        onVisualCue?.("STOP_FAIL_BLINK");

        attempts += 1;

        if (attempts >= MAX_ATTEMPTS) {
          resolveTest(false, "Recovery failed and max attempts reached", {
            attempts,
          });
        } else {
          resolveTest(false, "Recovery failed", {
            attempts,
          });
        }
      }

      setTimeout(tick, 100);
    };

    tick();
  });
};

export const testFunctions = [
  testCase1,
  testCase2,
  testCase3,
  testCase4,
  testCase5,
];
