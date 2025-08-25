import {
  ledOn,
  ledOff,
  startPulseLED,
  flashLED,
  getActiveFlashIntervals,
  setActiveFlashIntervals,
  resetTestCaseLEDs,
  resetStatusLEDs,
  clearActiveFlashIntervals,
} from "./leds.js";
import { systemState } from "./state.js";

let bootSequenceTimeouts = [];

export const clearBootTimeouts = () => {
  bootSequenceTimeouts.forEach(clearTimeout);
  bootSequenceTimeouts = [];
};

export const powerOnSequence = (statusLEDs) => {
  clearBootTimeouts();
  clearActiveFlashIntervals();
  const { pwr, init, rdy, run, idle } = statusLEDs;
  ledOn(pwr);
  const pwrOnTimeout1 = setTimeout(() => {
    const interval = flashLED(init, "red", 200, 2000);
    getActiveFlashIntervals().push(interval);
    const pwrOnTimeout2 = setTimeout(() => {
      ledOn(rdy);
      ledOff(run);
      startPulseLED(idle, 1500);
      systemState.ready = true;
      systemState.idle = true;
      systemState.running = false;
    }, 2000);
    bootSequenceTimeouts.push(pwrOnTimeout2);
  }, 500);
  bootSequenceTimeouts.push(pwrOnTimeout1);
};

export const startBootSequence = (statusLEDs) => {
  clearBootTimeouts();
  clearActiveFlashIntervals();
  console.log("Starting boot sequence...");
  if (!systemState.power) return;
  const { pwr, init, rdy, run, idle } = statusLEDs;
  ledOn(pwr);
  systemState.init = true;
  const initFlashDuration = 2000;
  const interval = flashLED(init, "red", 200, initFlashDuration);
  getActiveFlashIntervals().push(interval);
  const timeout = setTimeout(() => {
    if (!systemState.power) return;
    systemState.init = false;
    systemState.ready = true;
    systemState.idle = true;
    ledOn(rdy);
    startPulseLED(idle, 500, "red");
  }, initFlashDuration);
  bootSequenceTimeouts.push(timeout);
};

export const stopBootSequence = (statusLEDs, testCaseLEDs) => {
  console.log("Stopping boot sequence... (power off)");
  clearBootTimeouts();
  clearActiveFlashIntervals();
  resetStatusLEDs(statusLEDs);
  resetTestCaseLEDs(testCaseLEDs);
};

// Reset system logic
export const resetSystem = (statusLEDs, testCaseLEDs) => {
  if (!systemState.power) return;
  clearBootTimeouts();
  clearActiveFlashIntervals();
  console.log("Resetting system...");
  systemState.currentTestIndex = 0;
  systemState.testRunId++;
  //   stopPulseLED();
  getActiveFlashIntervals().forEach(clearInterval);
  setActiveFlashIntervals([]);
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
