// LED control functions for the simulator

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

export const isLEDOn = (element, expectedColour = "red") => {
  return element?.style.fill === expectedColour;
};

// Pulse, flash, and related LED helpers can be moved here as well in the next step.
