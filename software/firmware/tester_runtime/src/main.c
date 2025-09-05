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

typedef enum {
    TC_NO_RESULT,
    TC_PASS,
    TC_FAIL,
    TC_IN_PROGRESS,
    TC_RETRY
} TestCaseState;

volatile bool runMode      = false;  // false = IDLE, true = RUN
volatile bool flashState   = false;  // toggles every 250 ms
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

// --- Explicit port/mask map for columns and rows ---
static const struct { GPIO_TypeDef* port; uint16_t mask; } COL[5] = {
    {GPIOC, BIT(0)}, {GPIOC, BIT(1)}, {GPIOC, BIT(2)}, {GPIOC, BIT(3)}, {GPIOC, BIT(4)}
};
static const struct { GPIO_TypeDef* port; uint16_t mask; } ROW_RED   = { GPIOA, BIT(1) };
static const struct { GPIO_TypeDef* port; uint16_t mask; } ROW_GREEN = { GPIOA, BIT(2) };


// -------------------- Hard fault handler --------------------

void HardFault_Handler(void)
{
    __disable_irq();
    asm volatile ("csrr %0, mcause" : "=r"(last_mcause));
    asm volatile ("csrr %0, mepc"   : "=r"(last_mepc));
    for (;;) { /* spin */ }
}

// void HardFault_Handler(void)
// {
//     asm volatile ("csrr %0, mcause" : "=r"(last_mcause));
//     asm volatile ("csrr %0, mepc"   : "=r"(last_mepc));

//     // Rapid flash INIT so it's visible
//     while (1) {
//         funDigitalWrite(PIN_INIT, FUN_LOW);
//         for (volatile int i = 0; i < 60000; i++) {};
//         funDigitalWrite(PIN_INIT, FUN_HIGH);
//         for (volatile int i = 0; i < 60000; i++) {};
//     }
// }

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


// static inline void scanStep(void) {
//     static bool rdy = false;
//     rdy = !rdy;
//     funDigitalWrite(PIN_RDY, rdy ? FUN_LOW : FUN_HIGH); // active LOW
// }

// static inline void scanStep(void) {
//     gpio_set(COL[0].port, COL[0].mask); // active HIGH
// }

static inline void scanStep(void) {
    gpio_set(COL[1].port, COL[1].mask);
    gpio_clear(COL[1].port, COL[1].mask);
}


// Phased probe: one write per tick to find the wedging operation
// static inline void scanStep(void) {
//     static uint8_t phase = 0;      // advances 0..7 repeatedly
//     static int8_t  prev  = -1;     // previous column index
//     static uint8_t col   = 0;      // current column index (0..4)

//     // Show ISR progress by ticking RDY each tick (toggle)
//     static bool rdy = false;
//     rdy = !rdy;
//     funDigitalWrite(PIN_RDY, rdy ? FUN_LOW : FUN_HIGH); // active LOW

//     switch (phase) {
//         case 0:
//             // 0) rows OFF (active LOW -> set HIGH)
//             gpio_set(ROW_RED.port,   ROW_RED.mask);
//             break;

//         case 1:
//             // 1) rows OFF (green)
//             gpio_set(ROW_GREEN.port, ROW_GREEN.mask);
//             break;

//         case 2:
//             // 2) turn OFF previous column (if any)
//             if (prev >= 0) gpio_clear(COL[prev].port, COL[prev].mask);
//             break;

//         case 3:
//             // 3) drive rows for current column: PASS => green ON (LOW)
//             gpio_clear(ROW_GREEN.port, ROW_GREEN.mask);
//             break;

//         case 4:
//             // 4) turn ON current column (active HIGH)
//             gpio_set(COL[col].port, COL[col].mask);
//             break;

//         case 5:
//             // 5) do nothing (gap)
//             break;

//         case 6:
//             // 6) advance column index
//             prev = col;
//             col = (uint8_t)((col + 1) % 5);
//             break;

//         case 7:
//             // 7) do nothing (gap)
//             break;
//     }

//     // Advance to next phase each tick
//     phase = (uint8_t)((phase + 1) & 7);
// }


// static inline void scanStep(void) {
//     static int8_t prevCol = -1;

//     // Turn off previous column (active HIGH -> write LOW)
//     if (prevCol >= 0) {
//         // funDigitalWrite(testCols[prevCol], FUN_LOW);
//     }

//     // Default rows OFF (active LOW -> write HIGH)
//     // funDigitalWrite(PIN_ROW_R, FUN_HIGH);
//     // funDigitalWrite(PIN_ROW_G, FUN_HIGH);

//     // Drive rows for this column based on state
//     const  volatile TestCaseState* states = activeStates;
//     TestCaseState state = states[currentCol];

//     switch (state) {
//         case TC_NO_RESULT:
//             // both OFF
//             break;
//         case TC_PASS:
//             // funDigitalWrite(PIN_ROW_G, FUN_LOW);
//             break;
//         case TC_FAIL:
//             // funDigitalWrite(PIN_ROW_R, FUN_LOW);
//             break;
//         case TC_IN_PROGRESS:
//             if (flashState) {
//                 // funDigitalWrite(PIN_ROW_R, FUN_LOW);
//             }
//             else {
//                 // funDigitalWrite(PIN_ROW_G, FUN_LOW);
//             }
//             break;
//         case TC_RETRY:
//             if (currentCol == 4) { // only TC5 flashes red
//                 // if (flashState) funDigitalWrite(PIN_ROW_R, FUN_LOW);
//             }
//             break;
//     }

//     // Turn on this column last (active HIGH)
//     // funDigitalWrite(testCols[currentCol], FUN_HIGH);

//     // Advance
//     prevCol = currentCol;
//     currentCol = (currentCol + 1) % 5;
// }

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

// -------------------- Timer ISR --------------------

void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF)
    {
        // Clear update flag only
        TIM1->INTFR &= ~TIM_UIF;

        msTicks++;
        scanStep();

        // Flip flashState every 250 ms for TC blinking
        static uint32_t nextFlashAt = 250;
        if (msTicks >= nextFlashAt) {
            nextFlashAt += 250;
            flashState = !flashState;
        }
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



// static inline void setupPins(void) {
//     funGpioInitAll();
//     RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;


//     // Status LEDs (active LOW)
//     for (int i = 0; i < 5; i++) {
//         funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
//         funDigitalWrite(statusLEDs[i], FUN_HIGH); // OFF
//     }

//     // Test case columns (active HIGH)
//     for (int i = 0; i < 5; i++) {
//         funPinMode(testCols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
//         funDigitalWrite(testCols[i], FUN_LOW); // start OFF
//     }

//     // Test case rows (active LOW -> set HIGH to turn OFF)
//     for (int i = 0; i < 2; i++) {
//         funPinMode(testRows[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
//         funDigitalWrite(testRows[i], FUN_HIGH);
//     }

//     // Inputs with pull-ups
//     for (int i = 0; i < 4; i++) {
//         funPinMode(inputPins[i], GPIO_CNF_IN_PUPD);
//         funDigitalWrite(inputPins[i], FUN_HIGH);
//     }
// }

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
    runStatus(false);
}

// -------------------- App logic --------------------

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

// -------------------- Main --------------------

int main(void) {
    SystemInit();
    setupPins();
    setupStatusFlasher(); // start timer early for debug

    funDigitalWrite(PIN_ROW_G, FUN_LOW); // Active LOW
        
    funDigitalWrite(PIN_COL_A, FUN_HIGH);
    funDigitalWrite(PIN_COL_B, FUN_HIGH);
    funDigitalWrite(PIN_COL_C, FUN_HIGH); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_HIGH);
    funDigitalWrite(PIN_COL_E, FUN_HIGH);

    waitTicks(1000);

    funDigitalWrite(PIN_COL_A, FUN_LOW);
    funDigitalWrite(PIN_COL_B, FUN_LOW);
    funDigitalWrite(PIN_COL_C, FUN_LOW); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_LOW);
    funDigitalWrite(PIN_COL_E, FUN_LOW);

    waitTicks(1000);

    funDigitalWrite(PIN_COL_A, FUN_HIGH);
    funDigitalWrite(PIN_COL_B, FUN_HIGH);
    funDigitalWrite(PIN_COL_C, FUN_HIGH); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_HIGH);
    funDigitalWrite(PIN_COL_E, FUN_HIGH);

    waitTicks(1000);

    funDigitalWrite(PIN_COL_A, FUN_LOW);
    funDigitalWrite(PIN_COL_B, FUN_LOW);
    funDigitalWrite(PIN_COL_C, FUN_LOW); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_LOW);
    funDigitalWrite(PIN_COL_E, FUN_LOW);

    waitTicks(1000);
    
    funDigitalWrite(PIN_ROW_G, FUN_HIGH);  // Turn off Green
    funDigitalWrite(PIN_ROW_R, FUN_LOW);  // Turn on Red
    
    funDigitalWrite(PIN_COL_A, FUN_HIGH);
    funDigitalWrite(PIN_COL_B, FUN_HIGH);
    funDigitalWrite(PIN_COL_C, FUN_HIGH); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_HIGH);
    funDigitalWrite(PIN_COL_E, FUN_HIGH);

    waitTicks(1000);

    funDigitalWrite(PIN_COL_A, FUN_LOW);
    funDigitalWrite(PIN_COL_B, FUN_LOW);
    funDigitalWrite(PIN_COL_C, FUN_LOW); // active HIGH
    funDigitalWrite(PIN_COL_D, FUN_LOW);
    funDigitalWrite(PIN_COL_E, FUN_LOW);

    waitTicks(1000);

    initTestCaseStates();   // publish valid buffer before timer starts
    __enable_irq();         // enable global interrupts
    startupSequence();      // starts TIM1 and sets IDLE mode

    // runTestCaseDemo();

    while (1) {
        serviceStatusLeds();  // update RUN/IDLE outside ISR
        __WFI();
        static uint32_t lastBlink = 0;
if ((uint32_t)(msTicks - lastBlink) >= 200) {
    lastBlink += 200;
    static bool idleBlink = false;
    idleBlink = !idleBlink;
    funDigitalWrite(PIN_IDLE, idleBlink ? FUN_LOW : FUN_HIGH); // active LOW
}

    }
}
