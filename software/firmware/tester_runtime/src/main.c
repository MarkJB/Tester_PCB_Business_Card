#include "ch32fun.h"
#include <stdio.h>
#include <stdbool.h>

// Status Pins
#define PIN_PWR   PC5
#define PIN_INIT  PC6
#define PIN_RDY   PC7
#define PIN_RUN   PD0
#define PIN_IDLE  PD6

// Test Case Columns (TC1–TC5)
#define PIN_COL_A PC0
#define PIN_COL_B PC1
#define PIN_COL_C PC2
#define PIN_COL_D PC3
#define PIN_COL_E PC4

// Test Case Rows
#define PIN_ROW_R PA1  // Red (fail)
#define PIN_ROW_G PA2  // Green (pass)

// Input buttons
#define PIN_INPUT_A  PD2
#define PIN_INPUT_B  PD3
#define PIN_INPUT_C  PD4
#define PIN_INPUT_D  PD5

// Reset button is connected to NRST pin PD7 which should reset the MCU when pressed.
// This is a a hardware feature and does not require configuration.

const uint8_t statusLEDs[] = { PIN_PWR, PIN_INIT, PIN_RDY, PIN_RUN, PIN_IDLE };
const uint8_t testCols[] = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };
const uint8_t testRows[] = { PIN_ROW_R, PIN_ROW_G };
const uint8_t rowRed     = PIN_ROW_R;
const uint8_t rowGreen   = PIN_ROW_G;

const uint8_t inputPins[] = { PIN_INPUT_A, PIN_INPUT_B, PIN_INPUT_C, PIN_INPUT_D };

#define FLASH_INTERVAL_MS 10  // Flash every 10ms

volatile bool runMode = true;       // Current mode: true = RUN, false = IDLE
volatile bool ledState = false;     // Current LED state (on/off)

typedef enum {
    TC_NO_RESULT,
    TC_PASS,
    TC_FAIL,
    TC_IN_PROGRESS,
    TC_RETRY
} TestCaseState;

volatile TestCaseState testCaseStates[5];  // TC_PASS, TC_FAIL, etc.

volatile uint16_t tickCount = 0;
volatile bool flashState = false;

void scanTestCaseLEDs(void);

void initTestCaseStates() {
    for (int i = 0; i < 5; i++) {
        testCaseStates[i] = TC_NO_RESULT;
    }
}

// Forward declare with correct attribute (WCH RISC-V ISR)
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF)
    {
        TIM1->INTFR &= ~TIM_UIF;
        tickCount++;

        // Fast task: scan test case LEDs every tick
        scanTestCaseLEDs();

        // Slow task: toggle status LED every 500ms (50 ticks)
        if (tickCount >= 50) {
            tickCount = 0;
            flashState = !flashState;

            uint8_t activePin   = runMode ? PIN_RUN  : PIN_IDLE;
            uint8_t inactivePin = runMode ? PIN_IDLE : PIN_RUN;

            funDigitalWrite(activePin,   flashState ? FUN_LOW : FUN_HIGH);
            funDigitalWrite(inactivePin, FUN_HIGH);
        }
    }
}

// Non-blocking delay using tickCount
void waitTicks(uint16_t ticks) {
    uint16_t start = tickCount;
    while ((tickCount - start) < ticks) {
        // Let ISR run
        printf(".");
    }
}


// Initialize status LED pins and TIM1 for periodic toggling
void setupStatusFlasher(void)
{
    // Enable and reset TIM1 (on APB2)
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

    // 48MHz core -> 1kHz timer tick
    TIM1->PSC   = 48000 - 1;                 // prescaler to 1 kHz
    TIM1->ATRLR = FLASH_INTERVAL_MS - 1;     // ARR = 499 -> 500ms
    TIM1->SWEVGR |= TIM_UG;                  // load PSC/ARR
    TIM1->INTFR  &= ~TIM_UIF;                // clear any pending update flag
    TIM1->DMAINTENR |= TIM_UIE;              // enable update interrupt

    NVIC_EnableIRQ(TIM1_UP_IRQn);            // enable TIM1 update IRQ in NVIC
    TIM1->CTLR1 |= TIM_CEN;                  // start TIM1
}

// Switch which status LED is flashing
void runStatus(bool isRun)
{
    runMode  = isRun;
    ledState = false;

    // Ensure both OFF immediately; ISR will take over
    funDigitalWrite(PIN_RUN,  FUN_HIGH);
    funDigitalWrite(PIN_IDLE, FUN_HIGH);
}

void setupPins() {
    funGpioInitAll();

    // Status LEDs (LEDs are active LOW)     
    for (int i = 0; i < 5; i++) {
        funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(statusLEDs[i], FUN_HIGH); // Ensure all status LEDs start OFF
    }

    // Test Case LEDs

    // Columns (active HIGH, but row must be LOW to light LED)
    for (int i = 0; i < 5; i++) {
        funPinMode(testCols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(testCols[i], FUN_LOW); // Start all columns LOW
    }

    // Rows (active LOW)
    for (int i = 0; i < 2; i++) {
        funPinMode(testRows[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(testRows[i], FUN_HIGH); // Start all columns LOW
    }
}


int buttonPressed() {
    // Returns the index of the button pressed (0-3) or -1 if none pressed
    for (int i = 0; i < 4; i++) {
        if (funDigitalRead(inputPins[i]) == FUN_LOW) {
            return i;
        }
    }
    return -1;
}


void startupSequence() {
    funDigitalWrite(PIN_PWR, FUN_LOW); // PWR stays on
    // blink INIT for 3 times
    for (int i = 0; i < 3; i++) {
        funDigitalWrite(PIN_INIT, FUN_LOW);
        Delay_Ms(200);
        funDigitalWrite(PIN_INIT, FUN_HIGH);
        Delay_Ms(200);
    }
    // Turn on RDY
    funDigitalWrite(PIN_RDY, FUN_LOW);
    // Turn on IDLE (flashing)
    setupStatusFlasher();  // Start timer
    runStatus(false); // Set IDLE flashing
}

// Set the test case results via wrapper function
void setTestCaseResult(TestCaseState states[5]) {
    for (int i = 0; i < 5; i++) {
        testCaseStates[i] = states[i];
    }
}

// Scan and update the test case LEDs; to be called periodically via timer or main loop
void scanTestCaseLEDs() {
    for (int i = 0; i < 5; i++) {
        uint8_t col = testCols[i];
        TestCaseState state = testCaseStates[i];

        // Activate column
        funDigitalWrite(col, FUN_HIGH);

        switch (state) {
            case TC_NO_RESULT:
                // Both rows OFF
                funDigitalWrite(rowRed,   FUN_HIGH);
                funDigitalWrite(rowGreen, FUN_HIGH);
                break;
            case TC_PASS:
                funDigitalWrite(rowRed,   FUN_HIGH);
                funDigitalWrite(rowGreen, FUN_LOW);
                break;

            case TC_FAIL:
                funDigitalWrite(rowRed,   FUN_LOW);
                funDigitalWrite(rowGreen, FUN_HIGH);
                break;

            case TC_IN_PROGRESS:
                // Alternate flashing: handled by timer toggling a flag
                if (flashState) {
                    funDigitalWrite(rowRed,   FUN_LOW);
                    funDigitalWrite(rowGreen, FUN_HIGH);
                } else {
                    funDigitalWrite(rowRed,   FUN_HIGH);
                    funDigitalWrite(rowGreen, FUN_LOW);
                }
                break;

            case TC_RETRY:
                if (i == 4) {
                    funDigitalWrite(rowRed,   flashState ? FUN_LOW : FUN_HIGH);
                    funDigitalWrite(rowGreen, FUN_HIGH);  // Always OFF
                }
                break;
        }

        Delay_Ms(2);  // Short dwell time per column

        // Deactivate column
        funDigitalWrite(col, FUN_LOW);
    }

    // Reset rows to avoid ghosting
    funDigitalWrite(rowRed,   FUN_HIGH);
    funDigitalWrite(rowGreen, FUN_HIGH);
}


void runTestCaseDemo() {
    TestCaseState demoStates[5];

    demoStates[0] = TC_PASS;
    demoStates[1] = TC_FAIL;
    demoStates[2] = TC_IN_PROGRESS;
    demoStates[3] = TC_PASS;
    demoStates[4] = TC_RETRY;
    setTestCaseResult(demoStates);

    // Delay_Ms(5000);  // Initial 2 second delay
    // waitTicks(500); // 5 seconds

    demoStates[0] = TC_FAIL;
    demoStates[1] = TC_PASS;
    demoStates[2] = TC_FAIL;
    demoStates[3] = TC_RETRY;
    demoStates[4] = TC_IN_PROGRESS;
    setTestCaseResult(demoStates);
    waitTicks(5); // 5 seconds
    setTestCaseResult((TestCaseState[]){ TC_PASS, TC_PASS, TC_PASS, TC_PASS, TC_PASS } );
    setTestCaseResult((TestCaseState[]){ TC_FAIL, TC_FAIL, TC_FAIL, TC_FAIL, TC_FAIL } );
}


int main() {
    SystemInit();
    __enable_irq();           // Allow ISRs globally

    printf("Tester PCB Business Card Runtime\n");
    setupPins();
    startupSequence();
    printf("Running test case demo\n");
    runTestCaseDemo();
    printf("Demo complete. Entering main loop.\n");
    while (1) {
        // Do main loop tasks here

    }
}
