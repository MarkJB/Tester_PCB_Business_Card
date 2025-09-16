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
        // Each bit in pattern[col] corresponds to a status LED (active low)
        // Bit 0: PIN_PWR, Bit 1: PIN_INIT, Bit 2: PIN_RDY, Bit 3: PIN_RUN, Bit 4: PIN_IDLE
        // Set all status LEDs OFF (inactive/high)
        GPIO_SetBits(PIN_PWR);
        GPIO_SetBits(PIN_INIT);
        GPIO_SetBits(PIN_RDY);
        GPIO_SetBits(PIN_RUN);
        GPIO_SetBits(PIN_IDLE);

        if (pattern && (pattern[col] & 0x1F)) {
            if (pattern[col] & (1 << 0)) GPIO_ResetBits(PIN_PWR);   // ON (active low)
            if (pattern[col] & (1 << 1)) GPIO_ResetBits(PIN_INIT);
            if (pattern[col] & (1 << 2)) GPIO_ResetBits(PIN_RDY);
            if (pattern[col] & (1 << 3)) GPIO_ResetBits(PIN_RUN);
            if (pattern[col] & (1 << 4)) GPIO_ResetBits(PIN_IDLE);
        }
        Delay_Ms(10); // Adjust timing for PoV effect
    }
    // Always add a 1-column gap after each character (except if already a gap)
    if (numCols == 4) {
        GPIO_SetBits(PIN_PWR);
        GPIO_SetBits(PIN_INIT);
        GPIO_SetBits(PIN_RDY);
        GPIO_SetBits(PIN_RUN);
        GPIO_SetBits(PIN_IDLE);
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
