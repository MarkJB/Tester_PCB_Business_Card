


#include <stdint.h>
#include <stdbool.h>
#include "ch32fun.h"
#include "hardware.h"
#include "test_cases.h"
#include "globals.h"
#include "pins.h"

// Double-buffered states (publish by pointer swap)
static TestCaseState bufA[5], bufB[5];
volatile const TestCaseState* activeStates = bufA;
static uint8_t activeIdx = 0;



void initTestCaseStates(void) {
    for (int i = 0; i < 5; i++) { bufA[i] = bufB[i] = TC_NO_RESULT; }
    activeStates = bufA;
    activeIdx = 0;
}

void setTestCaseResult(const TestCaseState states[5]) {
    TestCaseState* dst = (activeIdx == 0) ? bufB : bufA;
    for (int i = 0; i < 5; i++) dst[i] = states[i];
    __disable_irq();
    __asm volatile ("" ::: "memory");
    activeStates = (volatile const TestCaseState*)dst;
    activeIdx ^= 1;
    __asm volatile ("" ::: "memory");
    __enable_irq();
}

void serviceStatusLeds(void) {
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

void turnOffAllStatusLeds(void) {
    funDigitalWrite(PIN_PWR, FUN_HIGH);
    funDigitalWrite(PIN_INIT, FUN_HIGH);
    funDigitalWrite(PIN_RDY, FUN_HIGH);
    funDigitalWrite(PIN_RUN, FUN_HIGH);
    funDigitalWrite(PIN_IDLE, FUN_HIGH);    
}

void testCaseLEDStartupPattern(void) {
    static TestCaseState demoStates[5];
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
            demoStates[i] = TC_PASS;
            setTestCaseResult(demoStates);
            waitTicks(30);
        }
        for (int i = 4; i >= 0; i--) {
            for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
            demoStates[i] = TC_FAIL;
            setTestCaseResult(demoStates);
            waitTicks(30);
        }
    }
    for (int j = 0; j < 5; j++) demoStates[j] = TC_NO_RESULT;
    setTestCaseResult(demoStates);
}
