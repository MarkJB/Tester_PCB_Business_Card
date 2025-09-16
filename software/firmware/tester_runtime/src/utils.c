#include <string.h> // For strlen
#include "ch32fun.h"
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

// Space is a special case 
static const uint8_t pov_font_space[1] = {
    0b00000, // Column 0
};

static const char* povMessage = "BUG";

void displayPOVChar(char c) {
    TestCaseState povStates[5];
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
        case ' ': // gap/space
            pattern = pov_font_space;
            numCols = 1;
            break;
        default:
            pattern = pov_font_space;
            numCols = 1;
            break;
    }
    // Display each column of the character
    for (int col = 0; col < numCols; col++) {
        for (int i = 0; i < 5; i++) povStates[i] = TC_NO_RESULT;
        if (pattern && (pattern[col] & 0x1F)) { // 5 bits for 5 LEDs
            for (int row = 0; row < 5; row++) {
                if (pattern[col] & (1 << row)) {
                    povStates[row] = TC_PASS;
                }
            }
        }
        setTestCaseResult(povStates);
        Delay_Ms(10); // Adjust timing for PoV effect
    }
    // Always add a 1-column gap after each character (except if already a gap)
    if (numCols == 4) {
        for (int i = 0; i < 5; i++) povStates[i] = TC_NO_RESULT;
        setTestCaseResult(povStates);
        Delay_Ms(5);
    }
}

void triggerPOVEasterEgg(void) {
    // turn off all other LEDs including PWR, INIT, RDY
    turnOffAllStatusLeds();

    while (1) {
        for (size_t i = 0; i < strlen(povMessage); i++) {
                displayPOVChar(povMessage[i]);
                // Delay_Ms(20); // Adjust timing for motion blur
            }
    }
    
}
