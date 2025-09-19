#include <stdint.h>
#include "test_cases.h"

extern volatile const TestCaseState* activeStates;
#ifndef LEDS_H
#define LEDS_H

#include <stdint.h>
#include <stdbool.h>


#include "test_cases.h"

void serviceStatusLeds(void);
void turnOffAllStatusLeds(void);
void setTestCaseResult(const TestCaseState states[5]);
void initTestCaseStates(void);
void testCaseLEDStartupPattern(void);
void demoMode(void);

#endif // LEDS_H
