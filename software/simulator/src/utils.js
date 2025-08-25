// Utility functions for the simulator (assertions, test results, etc.)

export const createTestResult = (passed, message = "", details = {}) => ({
  passed,
  message,
  details,
});

export const assert = (condition, message) => {
  if (!condition) throw new Error(message);
};

export const assertResult = (condition, message, details = {}) => {
  try {
    assert(condition, message);
    return createTestResult(true, message, details);
  } catch (err) {
    return createTestResult(false, err.message, details);
  }
};
