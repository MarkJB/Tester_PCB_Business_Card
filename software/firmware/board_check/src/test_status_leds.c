#include "ch32fun.h"
#include <stdio.h>

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

const uint8_t statusLEDs[] = { PIN_INIT, PIN_RDY, PIN_RUN, PIN_IDLE };
const uint8_t cols[] = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };

void setupPins() {
    funGpioInitAll();

    // Status LEDs
    funPinMode(PIN_PWR,   GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    for (int i = 0; i < 4; i++) {
        funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    }

    // Test Case LEDs
    for (int i = 0; i < 5; i++) {
        funPinMode(cols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    }
    funPinMode(PIN_ROW_R, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(PIN_ROW_G, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
}

void blinkStatusSequence() {
    funDigitalWrite(PIN_PWR, FUN_LOW); // PWR stays on

    for (int i = 0; i < 4; i++) {
        funDigitalWrite(statusLEDs[i], FUN_LOW);
        Delay_Ms(200);
        funDigitalWrite(statusLEDs[i], FUN_HIGH);
        Delay_Ms(100);
    }
}

void scanTestCases() {
    for (int i = 0; i < 5; i++) {
        // Reset all columns and rows
        for (int j = 0; j < 5; j++) funDigitalWrite(cols[j], FUN_LOW);
        funDigitalWrite(PIN_ROW_G, FUN_HIGH);
        funDigitalWrite(PIN_ROW_R, FUN_HIGH);

        // PASS (green)
        funDigitalWrite(cols[i], FUN_HIGH);
        funDigitalWrite(PIN_ROW_G, FUN_LOW);
        Delay_Ms(200);

        // FAIL (red)
        funDigitalWrite(PIN_ROW_G, FUN_HIGH);
        funDigitalWrite(PIN_ROW_R, FUN_LOW);
        Delay_Ms(200);

        // Reset
        funDigitalWrite(cols[i], FUN_LOW);
        funDigitalWrite(PIN_ROW_R, FUN_HIGH);
    }
}

int main() {
    SystemInit();
    setupPins();

    while (1) {
        blinkStatusSequence();
        scanTestCases();
        Delay_Ms(500); // Pause before repeating
    }
}
