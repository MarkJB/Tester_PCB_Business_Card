#include <string.h> // For strlen
#include "ch32fun.h"
#include "pins.h"
#include "utils.h"
#include "test_cases.h"
#include "leds.h"

bool allTestsPassed(void) {
    for (size_t i = 0; i < NUM_TEST_CASES; i++) {
        if (tcResults[i] != TC_PASS) return false;
    }
    return true;
}

// PoV Easter Egg
// Simple 5x4 font for PoV
// Message shows BUG in simple font using the following  4x5 font data
// B = 11111, 10101, 10101, 01010,
// U = 01111, 10000, 10000, 01111, 
// G = 01110, 10001, 11001, 01010,
// Gap = 00000

static const uint8_t pov_font_B[5] = {
    0b11111, // Column 0
    0b10101, // Column 1
    0b10101, // Column 2
    0b01010, // Column 3
};

static const uint8_t pov_font_U[5] = {
    0b01111, // Column 0
    0b10000, // Column 1
    0b10000, // Column 2
    0b01111, // Column 0
};

static const uint8_t pov_font_G[5] = {
    0b01110, // Column 0
    0b10001, // Column 1
    0b11001, // Column 2
    0b01010, // Column 3
};

// M
static const uint8_t pov_font_M[5] = {
    0b11111, // Column 0
    0b00010, // Column 1
    0b00100, // Column 2
    0b00010, // Column 1
    0b11111, // Column 0
};

// O is also used for 0
static const uint8_t pov_font_O[5] = {
    0b01110, // Column 0
    0b10001, // Column 1
    0b10001, // Column 2
    0b01110, // Column 3
};

// T
static const uint8_t pov_font_T[5] = {
    
    0b00001, // Column 1
    0b00001, // Column 2
    0b11111, // Column 3
    0b00001, // Column 4
    0b00001, // Column 5
};

// 2
static const uint8_t pov_font_2[5] = {
    0b11010, // Column 0
    0b11001, // Column 1
    0b11101, // Column 2
    0b10110, // Column 3
};

// 5
static const uint8_t pov_font_5[5] = {
    0b10111, // Column 0
    0b10101, // Column 1
    0b10101, // Column 2
    0b01001, // Column 3
};

// Space is a special case 
static const uint8_t pov_font_space[1] = {
    0b00000, // Column 0
};

static const char* povMessage = "MOT 2025 ";

void displayPOVChar(char c) {
    const uint8_t* pattern = NULL;
    int numCols = 0;
    // Select pattern and number of columns for each character
    switch (c) {
        case 'B':
            pattern = pov_font_B;
            numCols = 4;
            break;
        case 'U':
            pattern = pov_font_U;
            numCols = 4;
            break;
        case 'G':
            pattern = pov_font_G;
            numCols = 4;
            break;
        case 'M':
            pattern = pov_font_M;
            numCols = 5;
            break;
        case 'O':
            pattern = pov_font_O;
            numCols = 4;
            break;
        case 'T':
            pattern = pov_font_T;
            numCols = 5;
            break;
        case '0':
            pattern = pov_font_O;
            numCols = 4;
            break;
        case '2':
            pattern = pov_font_2;
            numCols = 4;
            break;
        case '5':
            pattern = pov_font_5;
            numCols = 4;
            break;            
        case ' ': // gap/space
            pattern = pov_font_space;
            numCols = 1;
            break;
        default:
            pattern = pov_font_space;
            numCols = 1;
            break;
    }
    // Display each column of the character using status LEDs (active low)
    for (int col = 0; col < numCols; col++) {
        // Set all status LEDs OFF (inactive/high)
        funDigitalWrite(PIN_PWR, 1);
        funDigitalWrite(PIN_INIT, 1);
        funDigitalWrite(PIN_RDY, 1);
        funDigitalWrite(PIN_RUN, 1);
        funDigitalWrite(PIN_IDLE, 1);

        if (pattern && (pattern[col] & 0x1F)) {
            if (pattern[col] & (1 << 0)) funDigitalWrite(PIN_PWR, 0);   // ON (active low)
            if (pattern[col] & (1 << 1)) funDigitalWrite(PIN_INIT, 0);
            if (pattern[col] & (1 << 2)) funDigitalWrite(PIN_RDY, 0);
            if (pattern[col] & (1 << 3)) funDigitalWrite(PIN_RUN, 0);
            if (pattern[col] & (1 << 4)) funDigitalWrite(PIN_IDLE, 0);
        }
        Delay_Ms(2); // Adjust timing for PoV effect
    }
    // Always add a 1-column gap after each character
    funDigitalWrite(PIN_PWR, 1);
    funDigitalWrite(PIN_INIT, 1);
    funDigitalWrite(PIN_RDY, 1);
    funDigitalWrite(PIN_RUN, 1);
    funDigitalWrite(PIN_IDLE, 1);
    Delay_Ms(2);
}

void triggerPOVEasterEgg(void) {
    // turn off all other LEDs including PWR, INIT, RDY
    turnOffAllStatusLeds();
    // turn off test case LEDs
    setTestCaseResult((TestCaseState[5]){ TC_NO_RESULT, TC_NO_RESULT, TC_NO_RESULT, TC_NO_RESULT, TC_NO_RESULT });

    while (1) {
        for (size_t i = 0; i < strlen(povMessage); i++) {
                displayPOVChar(povMessage[i]);
            }
    }
    
}
