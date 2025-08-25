import {
  ledOn,
  ledOff,
  startPulseLED,
  flashLED,
  activeFlashIntervals,
  resetTestCaseLEDs,
  resetStatusLEDs,
} from "./leds.js";
import { systemState } from "./state.js";

let bootSequenceTimeouts = [];

export const powerOnSequence = (statusLEDs) => {
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

export const startBootSequence = (statusLEDs) => {
  if (!systemState.power) return;
  const { pwr, init, rdy, run, idle } = statusLEDs;
  ledOn(pwr);
  systemState.init = true;
  const initFlashDuration = 2000;
  const interval = flashLED(init, "red", 200, initFlashDuration);
  activeFlashIntervals.push(interval);
  const timeout = setTimeout(() => {
    if (!systemState.power) return;
    systemState.init = false;
    systemState.ready = true;
    systemState.idle = true;
    ledOn(rdy);
  }, initFlashDuration);
  bootSequenceTimeouts.push(timeout);
};

export const stopBootSequence = (statusLEDs, testCaseLEDs) => {
  bootSequenceTimeouts.forEach(clearTimeout);
  bootSequenceTimeouts = [];
  activeFlashIntervals.forEach(clearInterval);
  activeFlashIntervals = [];
  resetStatusLEDs(statusLEDs);
  resetTestCaseLEDs(testCaseLEDs);
};
