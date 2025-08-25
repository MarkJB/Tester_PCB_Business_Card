
import { assertResult } from "./utils.js";
// ...existing code...

export const testCase2 = ({ setup, teardown, onVisualCue }) => {
  const pressTimes = [];
  const FAST_THRESHOLD = 200;
  let handler;
  setup(async () => {
    handler = () => {
      const now = Date.now();
      pressTimes.push(now);
      console.log(`Button B pressed at ${now}`);
    };
    buttonInputs.b.addEventListener("click", handler);
    console.log("🔧 TestCase2 setup: listener for Button B attached");
  });
  teardown(async () => {
    if (handler) {
      buttonInputs.b.removeEventListener("click", handler);
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

export const testCase3 = ({ setup, teardown, onVisualCue }) => {
  const inputState = { a: 0, b: 0, c: 0 };
  const requiredButtons = ["a", "b", "c"];
  const inputWindow = 5000;
  const listeners = [];
  setup(async () => {
    requiredButtons.forEach((key) => {
      const el = buttonInputs[key];
      const handler = () => {
        if (!systemState.ready) return;
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

export const testCase4 = ({ setup, teardown, onVisualCue }) => {
  const inputWindow = 5000;
  const pressCounts = { a: 0, b: 0, c: 0 };
  const listeners = [];
  const validRanges = {
    a: (count) => count >= 1 && count <= 3,
    b: (count) => count >= 5 && count <= 7,
    c: (count) => count >= 2 && count <= 4,
  };
  const handlePress = (key) => {
    if (!systemState.ready) return;
    pressCounts[key] += 1;
  };
  setup(async () => {
    ["a", "b", "c"].forEach((key) => {
      const el = buttonInputs[key];
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

export const testFunctions = [testCase1, testCase2, testCase3, testCase4];
