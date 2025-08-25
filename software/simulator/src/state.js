// System state and helpers for the simulator

export const systemState = {
  power: false,
  init: false,
  ready: false,
  idle: false,
  running: false,
};

export const isSystemReady = () => {
  return systemState.ready && systemState.idle && !systemState.running;
};
