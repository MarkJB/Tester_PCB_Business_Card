#include "ch32fun.h"
#include <stdbool.h>

// -------------------- Pin definitions --------------------

// Status LEDs (active LOW)
#define PIN_PWR   PC5
#define PIN_INIT  PC6
#define PIN_RDY   PC7
#define PIN_RUN   PD0
#define PIN_IDLE  PD6

// Test Case Columns (TC1–TC5), active HIGH
#define PIN_COL_A PC0
#define PIN_COL_B PC1
#define PIN_COL_C PC2
#define PIN_COL_D PC3
#define PIN_COL_E PC4

// Test Case Rows (active LOW)
#define PIN_ROW_R PA1  // Red (fail)
#define PIN_ROW_G PA2  // Green (pass)

// Input buttons (assumed active LOW with pull-ups)
#define PIN_INPUT_A  PD2
#define PIN_INPUT_B  PD3
#define PIN_INPUT_C  PD4
#define PIN_INPUT_D  PD5

// -------------------- Timing --------------------

#define TICKS_PER_SECOND 1000  // 1 ms tick

// -------------------- Tables --------------------

static const uint16_t statusLEDs[] = { PIN_PWR, PIN_INIT, PIN_RDY, PIN_RUN, PIN_IDLE };
static const uint16_t testCols[]   = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };
static const uint16_t testRows[]   = { PIN_ROW_R, PIN_ROW_G };
static const uint16_t inputPins[]  = { PIN_INPUT_A, PIN_INPUT_B, PIN_INPUT_C, PIN_INPUT_D };

// -------------------- State and flags --------------------

typedef struct {
    uint32_t lastPressTime;
    uint32_t lastReleaseTime;
    uint16_t pressCount;
    bool     pressed;      // true while button is physically held down
    bool     justPressed;  // true only on the tick a press is first detected, until consumed
} ButtonState;

volatile ButtonState buttons[4];

static uint8_t lastButtonSample = 0xFF; // all released (active-low)


typedef enum {
    TC_NO_RESULT,
    TC_PASS,
    TC_FAIL,
    TC_WARNING,
    TC_IN_PROGRESS,
    TC_RETRY
} TestCaseState;

volatile bool runMode      = false;  // false = IDLE, true = RUN
volatile bool flashState   = false;  // toggles every 250 ms
volatile bool rapidFlashState   = false;  // toggles every 125 ms
volatile uint32_t msTicks  = 0;      // monotonic ms counter
volatile uint8_t currentCol = 0;     // scan index

// Double-buffered states (publish by pointer swap)
static TestCaseState bufA[5], bufB[5];
static volatile const TestCaseState* activeStates = bufA;
static uint8_t activeIdx = 0;

// Optional: trap registers for debug (RISC-V)
volatile uint32_t last_mcause = 0, last_mepc = 0;


// --- Direct GPIO helpers ---
#define BIT(n) (1U << (n))
static inline void gpio_set(GPIO_TypeDef* p, uint16_t m)   { p->BSHR = m; }
static inline void gpio_clear(GPIO_TypeDef* p, uint16_t m) { p->BCR  = m; }
static inline uint8_t gpio_get(GPIO_TypeDef* p, uint16_t m) {
    return (p->INDR & m) ? 1U : 0U;
}

// --- Explicit port/mask map for columns and rows ---
static const struct { GPIO_TypeDef* port; uint16_t mask; } COL[5] = {
    {GPIOC, BIT(0)}, {GPIOC, BIT(1)}, {GPIOC, BIT(2)}, {GPIOC, BIT(3)}, {GPIOC, BIT(4)}
};

static const struct { GPIO_TypeDef* port; uint16_t mask; } ROW_RED   = { GPIOA, BIT(1) };
static const struct { GPIO_TypeDef* port; uint16_t mask; } ROW_GREEN = { GPIOA, BIT(2) };

// port/mask map for input buttons
static const struct { GPIO_TypeDef* port; uint16_t mask; } BTN[4] = {
    {GPIOD, BIT(2)}, {GPIOD, BIT(3)}, {GPIOD, BIT(4)}, {GPIOD, BIT(5)}
};


// -------------------- Hard fault handler --------------------

void HardFault_Handler(void)
{
    __disable_irq();
    asm volatile ("csrr %0, mcause" : "=r"(last_mcause));
    asm volatile ("csrr %0, mepc"   : "=r"(last_mepc));
    for (;;) { /* spin */ }
}

// -------------------- Helpers --------------------

static inline void initTestCaseStates(void) {
    for (int i = 0; i < 5; i++) { bufA[i] = bufB[i] = TC_NO_RESULT; }
    activeStates = bufA;
    activeIdx = 0;
}

static inline void setTestCaseResult(const TestCaseState states[5]) {
    // Copy into the inactive buffer with IRQs enabled
    TestCaseState* dst = (activeIdx == 0) ? bufB : bufA;
    for (int i = 0; i < 5; i++) dst[i] = states[i];

    // Publish with a tiny critical section (pointer swap) + compiler barriers
    __disable_irq();
    __asm volatile ("" ::: "memory");
    activeStates = (volatile const TestCaseState*)dst;
    activeIdx ^= 1;
    __asm volatile ("" ::: "memory");
    __enable_irq();
}

static inline void scanStep(void) {
    static int8_t prev = -1;  // last active column, -1 = none yet

    // 0. Bail out early if state pointer is invalid
    volatile const TestCaseState* states = activeStates;
    if (states == NULL) {
        // Ensure all LEDs off
        gpio_set(ROW_RED.port,   ROW_RED.mask);
        gpio_set(ROW_GREEN.port, ROW_GREEN.mask);
        return;
    }

    // 1. Turn both rows OFF (active LOW → drive HIGH) before touching columns
    gpio_set(ROW_RED.port,   ROW_RED.mask);
    gpio_set(ROW_GREEN.port, ROW_GREEN.mask);

    // 2. Clear the previous column if it was valid
    if (prev >= 0 && prev < 5) {
        if (COL[prev].port != NULL) {
            gpio_clear(COL[prev].port, COL[prev].mask);
        }
    }

    // 3. Decide what to light for the current column (only if in range)
    if (currentCol >= 0 && currentCol < 5) {
        TestCaseState state = states[currentCol];

        switch (state) {
            case TC_PASS:
                gpio_clear(ROW_GREEN.port, ROW_GREEN.mask); // green ON
                break;
            case TC_FAIL:
                gpio_clear(ROW_RED.port, ROW_RED.mask);     // red ON
                break;
            case TC_IN_PROGRESS:
                if (flashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                else
                    gpio_clear(ROW_GREEN.port, ROW_GREEN.mask);
                break;
            case TC_WARNING:
                if (flashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                break;
            case TC_RETRY:
                if (rapidFlashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                break;
            case TC_NO_RESULT:
            default:
                // both rows stay off
                break;
        }

        // 4. Set the current column HIGH (active HIGH)
        if (COL[currentCol].port != NULL) {
            gpio_set(COL[currentCol].port, COL[currentCol].mask);
        }
    }

    // 5. Advance indices for next tick
    prev = currentCol;
    currentCol = (currentCol + 1) % 5;
}


static inline void waitTicks(uint32_t ticks) {
    uint32_t start = msTicks;
    while ((uint32_t)(msTicks - start) < ticks) {
        __WFI();
    }
}

static inline void serviceStatusLeds(void) {
    // Update RUN/IDLE (active-low) outside ISR
    static bool lastFlash = 0, lastMode = 0;
    if (flashState != lastFlash || runMode != lastMode) {
        uint16_t activePin   = runMode ? PIN_RUN  : PIN_IDLE;
        uint16_t inactivePin = runMode ? PIN_IDLE : PIN_RUN;
        funDigitalWrite(activePin,   flashState ? FUN_LOW : FUN_HIGH);
        funDigitalWrite(inactivePin, FUN_HIGH);
        lastFlash = flashState;
        lastMode  = runMode;
    }
}

static uint32_t initLedOffAt = 0;

static inline void pollButtons(void) {
    uint8_t sample = 0;
    for (int i = 0; i < 4; i++) {
        if (!gpio_get(BTN[i].port, BTN[i].mask)) {
            sample |= (1 << i);
        }
    }

    uint8_t changed = sample ^ lastButtonSample;
    if (changed) {
        for (int i = 0; i < 4; i++) {
            if ((changed & (1 << i)) && (sample & (1 << i))) {
                // Rising edge: Pressed
                buttons[i].pressed = true;       // level
                buttons[i].justPressed = true;   // new: one-shot event
                buttons[i].lastPressTime = msTicks;
                buttons[i].pressCount++;

                // Start INIT LED pulse
                gpio_clear(GPIOC, BIT(6)); // INIT LED on (active low)
                initLedOffAt = msTicks + 50; // 50 ms pulse

            } else if (changed & (1 << i)) {
                // Falling edge: Released
                buttons[i].pressed = false;
                buttons[i].lastReleaseTime = msTicks;
            }
        }
    }

    // End INIT LED pulse if time elapsed
    if (initLedOffAt && msTicks >= initLedOffAt) {
        gpio_set(GPIOC, BIT(6)); // INIT LED off
        initLedOffAt = 0;
    }

    lastButtonSample = sample;
}

// -------------------- Timer ISR --------------------

void TIM1_UP_IRQHandler(void) INTERRUPT_DECORATOR;
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF) {
        TIM1->INTFR &= ~TIM_UIF;
        msTicks++;
        pollButtons();
        static uint32_t nextFlashAt = 250;
        if (msTicks >= nextFlashAt) {
            nextFlashAt += 250;
            flashState = !flashState;
        }
                static uint32_t nextRapidFlashAt = 125;
        if (msTicks >= nextRapidFlashAt) {
            nextRapidFlashAt += 125;
            rapidFlashState = !rapidFlashState;
        }
        scanStep();
        
    }
}

// -------------------- Hardware setup --------------------

static inline void setupStatusFlasher(void)
{
    // Enable and reset TIM1 (APB2)
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

    // 48 MHz -> 1 MHz timer, ARR=1000-1 => 1 ms
    TIM1->PSC    = 48 - 1;
    TIM1->ATRLR  = 1000 - 1;
    TIM1->SWEVGR |= TIM_UG;
    TIM1->INTFR  &= ~TIM_UIF;
    TIM1->DMAINTENR |= TIM_UIE;

    NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->CTLR1 |= TIM_CEN;
}

static inline void runStatus(bool isRun)
{
    runMode = isRun;
    // ensure both OFF, main loop will handle blinking
    funDigitalWrite(PIN_RUN,  FUN_HIGH);
    funDigitalWrite(PIN_IDLE, FUN_HIGH);
}

static inline void setupPins(void) {
    funGpioInitAll();

    // Explicit clocks for ports we touch directly
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    // Status LEDs (active LOW)
    for (int i = 0; i < 5; i++) {
        funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(statusLEDs[i], FUN_HIGH); // OFF
    }

    // Columns configured via ch32fun (active HIGH, start OFF)
    for (int i = 0; i < 5; i++) {
        funPinMode(testCols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(testCols[i], FUN_LOW);
    }

    // Rows (active LOW → OFF = HIGH)
    funPinMode(PIN_ROW_R, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(PIN_ROW_G, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funDigitalWrite(PIN_ROW_R, FUN_HIGH);
    funDigitalWrite(PIN_ROW_G, FUN_HIGH);

    // Inputs with pull-ups
    for (int i = 0; i < 4; i++) {
        funPinMode(inputPins[i], GPIO_CNF_IN_PUPD);
        funDigitalWrite(inputPins[i], FUN_HIGH);
    } 
}

static inline void testCaseLEDStartupPattern(void) {
    static TestCaseState demoStates[5];

    // Repeat the above 3 times so it looks like a "chase"
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
            demoStates[i] = TC_PASS;
            setTestCaseResult(demoStates);
            waitTicks(30);
        }
        for (int i = 4; i >= 0; i--) {
            for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
            demoStates[i] = TC_FAIL;
            setTestCaseResult(demoStates);
            waitTicks(30);
        }
    }
    // end with all off
    for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
    setTestCaseResult(demoStates);
}

static inline void startupSequence(void) {
    funDigitalWrite(PIN_PWR, FUN_LOW); // PWR stays on

    // Blink INIT 3 times
    for (int i = 0; i < 3; i++) {
        funDigitalWrite(PIN_INIT, FUN_LOW);
        Delay_Ms(200);
        funDigitalWrite(PIN_INIT, FUN_HIGH);
        Delay_Ms(200);
    }

    // RDY on
    funDigitalWrite(PIN_RDY, FUN_LOW);

    // Start timer and set IDLE flashing
    setupStatusFlasher();
    __enable_irq();         // enable global interrupts
    runStatus(false);
    testCaseLEDStartupPattern();
}

static inline void runTestCaseDemo(void) {
    static TestCaseState demoStates[5];

    // Pattern 1
    demoStates[0] = TC_PASS;
    demoStates[1] = TC_FAIL;
    demoStates[2] = TC_IN_PROGRESS;
    demoStates[3] = TC_PASS;
    demoStates[4] = TC_RETRY;
    setTestCaseResult(demoStates);
    waitTicks(5 * TICKS_PER_SECOND);

    // Pattern 2
    demoStates[0] = TC_FAIL;
    demoStates[1] = TC_PASS;
    demoStates[2] = TC_FAIL;
    demoStates[3] = TC_RETRY;
    demoStates[4] = TC_IN_PROGRESS;
    setTestCaseResult(demoStates);
    waitTicks(5 * TICKS_PER_SECOND);

    // Pattern 3 (all pass)
    for (int i = 0; i < 5; i++) demoStates[i] = TC_PASS;
    setTestCaseResult(demoStates);
    waitTicks(5 * TICKS_PER_SECOND);

    // Pattern 4 (all fail)
    for (int i = 0; i < 5; i++) demoStates[i] = TC_FAIL;
    setTestCaseResult(demoStates);
}

// -------------------- App logic --------------------

static TestCaseState tcResults[5] = { TC_NO_RESULT };
typedef struct {
    uint32_t durationMs;
    void (*initFn)(void);
    void (*updateFn)(void);
    TestCaseState (*evalFn)(void);
} TestCaseDef;


static uint8_t currentTest = 0;
static uint32_t testStartTime = 0;
static bool testActive = false;

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
static uint32_t firstPressTimeB, lastPressTimeB;
static uint8_t pressCountB;
static const uint32_t DEBOUNCE_THRESHOLD_MS = 200;

// TC2: Fast double-press detection
static uint32_t pressTimes[4];
static uint8_t pressIndex;
static const uint32_t FAST_THRESHOLD_MS = 200;
static const uint32_t MIN_GAP_MS        = 30;  // reject bounce

static void tc2_init(void) {
    pressIndex = 0;
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
    if (pressCountB < 2) {
        return TC_FAIL; // no presses or only one press
    }

    uint32_t gap = lastPressTimeB - firstPressTimeB;

    if (gap < MIN_GAP_MS) {
        return TC_FAIL; // bounce
    }
    if (gap >= FAST_THRESHOLD_MS) {
        return TC_FAIL; // too slow
    }

    return TC_PASS; // valid fast double press
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
#define A_MIN 1
#define A_MAX 3
#define B_MIN 5
#define B_MAX 7
#define C_MIN 2
#define C_MAX 4

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

    // For now, map to overall PASS/FAIL/RETRY
    // You could extend this to show per‑input LEDs if hardware supports it
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

// ===== Test case framework =====


static const TestCaseDef testCases[] = {
    { 5000, tc1_init, tc1_update, tc1_eval },
    { 5000, tc2_init, tc2_update, tc2_eval },
    { 5000, tc3_init, tc3_update, tc3_eval },
    { 5000, tc4_init, tc4_update, tc4_eval },
};

static const size_t NUM_TEST_CASES = sizeof(testCases) / sizeof(testCases[0]);

static void startTest(uint8_t idx) {
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

    testCases[idx].initFn(); // per-test init
}

static void endTest(void) {
    testActive = false;
    runStatus(false);

    TestCaseState outcome = testCases[currentTest].evalFn();
    tcResults[currentTest] = outcome;
    setTestCaseResult(tcResults);

    currentTest = (currentTest + 1) % NUM_TEST_CASES;
}

static void monitorInputs(void) {
    if (!testActive) return;
    testCases[currentTest].updateFn(); // per-test input handling
    if ((uint32_t)(msTicks - testStartTime) >= testCases[currentTest].durationMs) {
        endTest();
    }
}

// -------------------- Main --------------------

int main(void) {
    SystemInit();

    setupPins();

    initTestCaseStates();   // publish valid buffer before timer starts
    
    startupSequence();      // starts TIM1 and sets IDLE mode

    waitTicks(100); // let it settle

    // runTestCaseDemo();
    // testCaseLEDStartupPattern();

    while (1) {
        serviceStatusLeds();  // update RUN/IDLE outside ISR
            // START TEST button = BTN D (index 3)
    
        if (!testActive && buttons[3].pressed) {
            startTest(currentTest);
        }

        monitorInputs();

        __WFI();
        }

}

