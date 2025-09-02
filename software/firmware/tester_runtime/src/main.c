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
const uint8_t cols[] = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };
const uint8_t rows[] = { PIN_ROW_R, PIN_ROW_G };
const uint8_t inputPins[] = { PIN_INPUT_A, PIN_INPUT_B, PIN_INPUT_C, PIN_INPUT_D };

#define FLASH_INTERVAL_MS 500  // Flash every 500ms

volatile bool runMode = true;       // Current mode: true = RUN, false = IDLE
volatile bool ledState = false;     // Current LED state (on/off)

typedef enum {
    TC_PASS,
    TC_FAIL,
    TC_IN_PROGRESS,
    TC_RETRY
} TestCaseState;


// Forward declare with correct attribute (WCH RISC-V ISR)
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

// TIM1 update interrupt handler
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF)
    {
        TIM1->INTFR &= ~TIM_UIF;  // clear update flag

        uint8_t activePin   = runMode ? PIN_RUN  : PIN_IDLE;
        uint8_t inactivePin = runMode ? PIN_IDLE : PIN_RUN;

        // Active-low LEDs: ON = LOW, OFF = HIGH
        ledState = !ledState;
        funDigitalWrite(activePin,   ledState ? FUN_LOW : FUN_HIGH);
        funDigitalWrite(inactivePin, FUN_HIGH);  // keep the other one OFF
    }
}

// Initialize status LED pins and TIM1 for periodic toggling
void setupStatusFlasher(void)
{
    // Pins already configured in setupPins(); ensure OFF to start
    funDigitalWrite(PIN_RUN,  FUN_HIGH);
    funDigitalWrite(PIN_IDLE, FUN_HIGH);

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
        funPinMode(cols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(cols[i], FUN_LOW); // Start all columns LOW
    }

    // Rows (active LOW)
    for (int i = 0; i < 2; i++) {
        funPinMode(rows[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(rows[i], FUN_HIGH); // Start all columns LOW
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

void setTestCaseResult() {
    // Set the test case result by an array of true or false values
}



int main() {
    SystemInit();
    __enable_irq();           // Allow ISRs globally

    setupPins();
    startupSequence();

    while (1) {
        // Do main loop tasks here
        while (1) {
    // ledState = !ledState;
    // funDigitalWrite(PIN_IDLE, ledState ? FUN_HIGH : FUN_LOW);
    // Delay_Ms(500);
}

    }
}
