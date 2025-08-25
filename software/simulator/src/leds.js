import { systemState } from "./state.js";

// Pulse and flash helpers
let _activeFlashIntervals = [];
export const getActiveFlashIntervals = () => {
  return _activeFlashIntervals;
};
export const setActiveFlashIntervals = (val) => {
  _activeFlashIntervals = val;
};

export const clearActiveFlashIntervals = () => {
  _activeFlashIntervals.forEach((entry) => {
    if (typeof entry === "number") {
      clearInterval(entry);
    } else if (entry && typeof entry.id === "number") {
      clearInterval(entry.id);
    }
  });
  _activeFlashIntervals = [];
};

export const startPulseLED = (ledElement, interval, colour = "red") => {
  let isOn = false;
  const pulse = setInterval(() => {
    if (!systemState?.power) {
      clearInterval(pulse);
      ledOff(ledElement);
      return;
    }
    isOn ? ledOff(ledElement) : ledOn(ledElement, colour);
    isOn = !isOn;
  }, interval);
  _activeFlashIntervals.push(pulse);
};

export const stopPulseLED = () => {
  clearActiveFlashIntervals();
};
// LED control functions for the simulator

export const testCaseInProgress = (passLED, failLED, interval = 500) => {
  let toggle = false;

  const flash = {
    id: setInterval(() => {
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
    }, interval),
    source: "testCaseInProgress",
  };

  _activeFlashIntervals.push(flash);
};

export const ledOn = (element, colour = "red") => {
  element.style.fill = colour;
  element.style.filter = `drop-shadow(0 0 6px ${colour})`;
  element.style.transition = "fill 0.1s ease-in-out, filter 0.1s ease-in-out";
};

export const ledOff = (element) => {
  element.style.fill = "white";
  element.style.filter = "none";
  element.style.transition = "fill 0.1s ease-in-out, filter 0.1s ease-in-out";
};

export const toggleLED = (element, colour = "red") => {
  if (isLEDOn(element, colour)) {
    ledOff(element);
  } else {
    ledOn(element, colour);
  }
};

export const flashLED = (
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

export const isLEDOn = (element, expectedColour = "red") => {
  return element?.style.fill === expectedColour;
};

export const statusRunning = (statusLEDs) => {
  stopPulseLED(); // clears any previous intervals
  if (statusLEDs.idle) ledOff(statusLEDs.idle); // ensure idle LED is off
  if (statusLEDs.run) startPulseLED(statusLEDs.run, 500); // fast pulse for active state

  systemState.idle = false;
  systemState.running = true;
};

export const statusIdle = (statusLEDs) => {
  stopPulseLED();
  if (statusLEDs.run) ledOff(statusLEDs.run);
  if (statusLEDs.idle) startPulseLED(statusLEDs.idle, 1500); // slow pulse for idle state

  systemState.idle = true;
  systemState.running = false;
};

export const resetTestCaseLEDs = (testCaseLEDs) => {
  stopPulseLED(); // clears any previous intervals
  testCaseLEDs.forEach(({ pass, fail }) => {
    ledOff(pass);
    ledOff(fail);
  });
};

export const resetStatusLEDs = (statusLEDs) => {
  Object.values(statusLEDs).forEach(ledOff);
};
