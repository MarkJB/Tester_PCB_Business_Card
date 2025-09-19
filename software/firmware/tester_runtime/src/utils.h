// utils.h – Easter egg and POV rendering utilities
// Exposes functions for test result checking and LED-based message display

#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>  // For bool
#include <stddef.h>   // For size_t
#include "test_cases.h" // For TestCaseState

extern const size_t NUM_TEST_CASES;
extern TestCaseState tcResults[];

// Public function declarations
bool allTestsPassed(void);
void displayPOVChar(char c);
void triggerPOVEasterEgg(void);

#endif // UTILS_H
