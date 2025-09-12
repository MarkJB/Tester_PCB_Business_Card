
#include "test_cases.h"
#include "buttons.h"
#include "leds.h"
#include "hardware.h"
#include "globals.h"
#include "pins.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

TestCaseState tcResults[5] = { TC_NO_RESULT };
uint8_t currentTest = 0;
bool testActive = false;
static uint32_t testStartTime = 0;


// ===== TC1: All A–C pressed =====
static bool seenA, seenB, seenC;

static void tc1_init(void) {
	seenA = seenB = seenC = false;
}

static void tc1_update(void) {
	if (buttons[0].pressed) seenA = true;
	if (buttons[1].pressed) seenB = true;
	if (buttons[2].pressed) seenC = true;
}

static TestCaseState tc1_eval(void) {
	return (seenA && seenB && seenC) ? TC_PASS : TC_FAIL;
}

// ===== TC2: Debounce timing =====
// TC2: Fast double-press detection
static uint32_t pressTimes[4];
static uint8_t pressIndex;
static const uint32_t FAST_THRESHOLD_MS = 200;
static const uint32_t MIN_GAP_MS        = 30;  // reject bounce

static void tc2_init(void) {
    pressIndex = 0;
    for (uint8_t i = 0; i < 4; i++) {
        pressTimes[i] = 0;
    }
}

static void tc2_update(void) {
	if (buttons[1].justPressed) {
		buttons[1].justPressed = false; // consume event
		if (pressIndex < 4) {
			pressTimes[pressIndex++] = msTicks;
		}
	}
}

static TestCaseState tc2_eval(void) {
    for (uint8_t i = 0; i < pressIndex - 1; i++) {
        uint32_t gap = pressTimes[i + 1] - pressTimes[i];

        if (gap >= MIN_GAP_MS && gap < FAST_THRESHOLD_MS) {
            return TC_PASS; // found a valid fast double press
        }
    }
    return TC_FAIL; // no valid pair found
}

// ===== TC3: Conditional Logic – Decision Table Test =====

// Latch states for A, B, C during the 5s window
static bool seenA3, seenB3, seenC3;

static void tc3_init(void) {
	seenA3 = seenB3 = seenC3 = false;
}

static void tc3_update(void) {
	if (buttons[0].justPressed) { 
		buttons[0].justPressed = false; 
		seenA3 = !seenA3; // toggle
	}
	if (buttons[1].justPressed) { 
		buttons[1].justPressed = false; 
		seenB3 = !seenB3; // toggle
	}
	if (buttons[2].justPressed) { 
		buttons[2].justPressed = false; 
		seenC3 = !seenC3; // toggle
	}
}

// Decision table returning your existing TestCaseState
static TestCaseState tc3_eval(void) {
	// Decision table mapping
	if (!seenA3 && !seenB3 && !seenC3) return TC_FAIL;   // 000
	if (!seenA3 && !seenB3 &&  seenC3) return TC_WARNING;  // 001
	if (!seenA3 &&  seenB3 && !seenC3) return TC_WARNING;  // 010
	if (!seenA3 &&  seenB3 &&  seenC3) return TC_PASS;   // 011 ✅
	if ( seenA3 && !seenB3 && !seenC3) return TC_FAIL;   // 100
	if ( seenA3 && !seenB3 &&  seenC3) return TC_WARNING;  // 101
	if ( seenA3 &&  seenB3 && !seenC3) return TC_WARNING;  // 110
	if ( seenA3 &&  seenB3 &&  seenC3) return TC_WARNING;  // 111
	return TC_FAIL;
}

// ===== TC4: Input Range Check =====

// Press counters for A, B, C
static uint8_t countA4, countB4, countC4;

// Valid ranges
#define A_MIN 5
#define A_MAX 7
// 6 is a pass
#define B_MIN 7
#define B_MAX 12
// 8 - 11 are passes
#define C_MIN 2
#define C_MAX 4
// 3 is a pass

static void tc4_init(void) {
	countA4 = countB4 = countC4 = 0;
}

static void tc4_update(void) {
	if (buttons[0].justPressed) { buttons[0].justPressed = false; countA4++; }
	if (buttons[1].justPressed) { buttons[1].justPressed = false; countB4++; }
	if (buttons[2].justPressed) { buttons[2].justPressed = false; countC4++; }
}

static TestCaseState tc4_eval(void) {
	bool validA = (countA4 >= A_MIN && countA4 <= A_MAX);
	bool validB = (countB4 >= B_MIN && countB4 <= B_MAX);
	bool validC = (countC4 >= C_MIN && countC4 <= C_MAX);

	bool anyValid = validA || validB || validC;

	if (!anyValid) {
		return TC_FAIL;
	}

	// Optional: treat borderline values (exactly at min or max) as RETRY
	bool borderlineA = validA && (countA4 == A_MIN || countA4 == A_MAX);
	bool borderlineB = validB && (countB4 == B_MIN || countB4 == B_MAX);
	bool borderlineC = validC && (countC4 == C_MIN || countC4 == C_MAX);

	if (borderlineA || borderlineB || borderlineC) {
		return TC_WARNING; // blink pattern for borderline
	}

	return TC_PASS;
}

// ===== TC5: Unlock Pattern – State Transition / Timeout + Recovery =====
static const uint8_t correctSeq[5] = { 0, 1, 2, 1, 0 };
#define MAX_INPUTS         5
#define MAX_ATTEMPTS       3
#define RECOVERY_WINDOW_MS 5000
static uint8_t inputSeq[MAX_INPUTS];
static uint8_t inputCount;
static uint8_t attempts;
static bool inRecovery;
static uint32_t recoveryStart;
static bool lastWasC;
static TestCaseState tc5Outcome;

static void tc5_init(void) {
	inputCount    = 0;
	attempts      = 0;
	inRecovery    = false;
	recoveryStart = 0;
	lastWasC      = false;
	tc5Outcome    = TC_IN_PROGRESS;
	tcResults[currentTest] = TC_IN_PROGRESS;
	setTestCaseResult(tcResults);
}

static void tc5_update(void) {
	if (inRecovery && (msTicks - recoveryStart >= RECOVERY_WINDOW_MS)) {
		attempts++;
		tc5Outcome = (attempts >= MAX_ATTEMPTS) ? TC_FAIL : TC_FAIL;
		inRecovery = false;
		tcResults[currentTest] = tc5Outcome;
		setTestCaseResult(tcResults);
		return;
	}
	for (uint8_t i = 0; i < 3; i++) {
		if (buttons[i].justPressed) {
			buttons[i].justPressed = false;
			if (inRecovery) {
				if (i == 2) {
					if (lastWasC) {
						inRecovery    = false;
						inputCount    = 0;
						lastWasC      = false;
						tc5Outcome    = TC_IN_PROGRESS;
						tcResults[currentTest] = TC_IN_PROGRESS;
						setTestCaseResult(tcResults);
						return;
					} else {
						lastWasC = true;
					}
				} else {
					lastWasC = false;
				}
				return;
			}
			if (inputCount < MAX_INPUTS) {
				inputSeq[inputCount++] = i;
				if (inputCount == MAX_INPUTS) {
					bool correct = true;
					for (uint8_t j = 0; j < MAX_INPUTS; j++) {
						if (inputSeq[j] != correctSeq[j]) {
							correct = false;
							break;
						}
					}
					if (correct) {
						tc5Outcome = TC_PASS;
						tcResults[currentTest] = TC_PASS;
						setTestCaseResult(tcResults);
					} else {
						attempts++;
						if (attempts >= MAX_ATTEMPTS) {
							tc5Outcome = TC_FAIL;
							tcResults[currentTest] = TC_FAIL;
							setTestCaseResult(tcResults);
						} else {
							inRecovery    = true;
							recoveryStart = msTicks;
							lastWasC      = false;
							tc5Outcome    = TC_RETRY;
							tcResults[currentTest] = TC_RETRY;
							setTestCaseResult(tcResults);
						}
					}
				}
			}
		}
	}
}

static TestCaseState tc5_eval(void) {
	if (tc5Outcome == TC_IN_PROGRESS) {
		return TC_FAIL;
	}
	return tc5Outcome;
}

typedef struct {
	uint32_t durationMs;
	void (*initFn)(void);
	void (*updateFn)(void);
	TestCaseState (*evalFn)(void);
} TestCaseDef;

static const TestCaseDef testCases[] = {
	{ 5000,  tc1_init, tc1_update, tc1_eval },
	{ 5000,  tc2_init, tc2_update, tc2_eval },
	{ 5000,  tc3_init, tc3_update, tc3_eval },
	{ 5000,  tc4_init, tc4_update, tc4_eval },
	// { 10000, tc5_init, tc5_update, tc5_eval }
};
static const size_t NUM_TEST_CASES = sizeof(testCases) / sizeof(testCases[0]);

void test_cases_start(uint8_t idx) {
	if (idx == 0) {
		for (size_t i = 0; i < NUM_TEST_CASES; i++) {
			tcResults[i] = TC_NO_RESULT;
		}
	}
	testStartTime = msTicks;
	testActive = true;
	runStatus(true);
	tcResults[idx] = TC_IN_PROGRESS;
	setTestCaseResult(tcResults);
	testCases[idx].initFn();
}

void test_cases_end(void) {
	testActive = false;
	runStatus(false);
	TestCaseState outcome = testCases[currentTest].evalFn();
	tcResults[currentTest] = outcome;
	setTestCaseResult(tcResults);
	currentTest = (currentTest + 1) % NUM_TEST_CASES;
}

void test_cases_monitor_inputs(void) {
	if (!testActive) return;
	testCases[currentTest].updateFn();
	if ((uint32_t)(msTicks - testStartTime) >= testCases[currentTest].durationMs) {
		test_cases_end();
	}
}

void test_cases_init(void) {}
void test_cases_update(void) {}
TestCaseState test_cases_eval(void) { return TC_NO_RESULT; }
