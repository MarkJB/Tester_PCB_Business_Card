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

const uint8_t statusLEDs[] = { PIN_INIT, PIN_RDY, PIN_RUN, PIN_IDLE };
const uint8_t cols[] = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };

typedef enum {
    MODE_DEMO,
    MODE_INPUT
} BoardMode;

BoardMode currentMode = MODE_DEMO;
void runInputMode(void);

void setupPins() {
    funGpioInitAll();

    // Status LEDs
    funPinMode(PIN_PWR, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funDigitalWrite(PIN_PWR, FUN_LOW); // Turn PWR ON immediately

    for (int i = 0; i < 4; i++) {
        funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(statusLEDs[i], FUN_HIGH); // Ensure all status LEDs start OFF
    }

    // Test Case LEDs
    for (int i = 0; i < 5; i++) {
        funPinMode(cols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(cols[i], FUN_LOW); // Start all columns LOW
    }

    funPinMode(PIN_ROW_R, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(PIN_ROW_G, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funDigitalWrite(PIN_ROW_R, FUN_HIGH); // Start rows OFF
    funDigitalWrite(PIN_ROW_G, FUN_HIGH);
}

const uint8_t inputPins[] = { PIN_INPUT_A, PIN_INPUT_B, PIN_INPUT_C, PIN_INPUT_D };
const uint8_t inputCols[] = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D }; // TC1–TC4

bool isAnyButtonPressed() {
    for (int i = 0; i < 4; i++) {
        if (funDigitalRead(inputPins[i]) == FUN_LOW) {
            return true;
        }
    }
    return false;
}

void delayWithInputCheck(uint16_t ms) {
    uint16_t step = 5;
    for (uint16_t i = 0; i < ms; i += step) {
        if (isAnyButtonPressed()) {
            currentMode = MODE_INPUT;
            runInputMode(); // never returns
        }
        Delay_Ms(step);
    }
}

void blinkStatusSequence() {
    funDigitalWrite(PIN_PWR, FUN_LOW); // PWR stays on

    for (int i = 0; i < 4; i++) {
        funDigitalWrite(statusLEDs[i], FUN_LOW);
        delayWithInputCheck(200); // or delayWithInputCheck(100)
        funDigitalWrite(statusLEDs[i], FUN_HIGH);
        delayWithInputCheck(200); // or delayWithInputCheck(100)
    }
}

void scanTestCases() {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) funDigitalWrite(cols[j], FUN_LOW);
        funDigitalWrite(PIN_ROW_G, FUN_HIGH);
        funDigitalWrite(PIN_ROW_R, FUN_HIGH);

        funDigitalWrite(cols[i], FUN_HIGH);
        funDigitalWrite(PIN_ROW_G, FUN_LOW);
        delayWithInputCheck(200); // or delayWithInputCheck(100)

        funDigitalWrite(PIN_ROW_G, FUN_HIGH);
        funDigitalWrite(PIN_ROW_R, FUN_LOW);
        delayWithInputCheck(200); // or delayWithInputCheck(100)

        funDigitalWrite(cols[i], FUN_LOW);
        funDigitalWrite(PIN_ROW_R, FUN_HIGH);
    }
}

void runInputMode() {
    // Turn off all LEDs except PWR
    for (int i = 0; i < 4; i++) funDigitalWrite(statusLEDs[i], FUN_HIGH);
    for (int i = 0; i < 5; i++) funDigitalWrite(cols[i], FUN_LOW);
    funDigitalWrite(PIN_ROW_R, FUN_HIGH);
    funDigitalWrite(PIN_ROW_G, FUN_HIGH);

    while (1) {
        bool anyPressed = false;

        for (int i = 0; i < 4; i++) {
            if (funDigitalRead(inputPins[i]) == FUN_LOW) {
                funDigitalWrite(inputCols[i], FUN_HIGH);
                anyPressed = true;
            } else {
                funDigitalWrite(inputCols[i], FUN_LOW);
            }
        }

        funDigitalWrite(PIN_ROW_G, anyPressed ? FUN_LOW : FUN_HIGH);
        Delay_Ms(50); // Responsive polling
    }
}

int main() {
    SystemInit();
    setupPins();

    while (1) {
        if (currentMode == MODE_DEMO) {
            if (isAnyButtonPressed()) {
                currentMode = MODE_INPUT;
                runInputMode(); // never returns
            }

            blinkStatusSequence();
            scanTestCases();
            Delay_Ms(500);
        }
    }
}
