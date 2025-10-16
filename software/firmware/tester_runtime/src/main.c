#include "utils.h"
// Prototypes for functions used but not declared
bool allTestsPassed(void);
void triggerPOVEasterEgg(void);
#include "ch32fun.h"
#include <stdbool.h>
#include "globals.h"
#include "pins.h"
#include "hardware.h"
#include "leds.h"
#include "buttons.h"
#include "test_cases.h"

// Optional: trap registers for debug (RISC-V)
volatile uint32_t last_mcause = 0, last_mepc = 0;

void HardFault_Handler(void)
{
    __disable_irq();
    asm volatile ("csrr %0, mcause" : "=r"(last_mcause));
    asm volatile ("csrr %0, mepc"   : "=r"(last_mepc));
    for (;;) { /* spin */ }
}

static void startupSequence(void) {
    // Power LED on, blink INIT, RDY on, start timer, set IDLE flashing, LED pattern
    funDigitalWrite(PIN_PWR, FUN_LOW); // PWR stays on
    for (int i = 0; i < 3; i++) {
        funDigitalWrite(PIN_INIT, FUN_LOW);
        Delay_Ms(200);
        funDigitalWrite(PIN_INIT, FUN_HIGH);
        Delay_Ms(200);
    }
    funDigitalWrite(PIN_RDY, FUN_LOW);
    setupStatusFlasher();
    __enable_irq();
    runStatus(false);
    testCaseLEDStartupPattern();
}

 int main(void) {
    SystemInit();
    setupPins();
    initTestCaseStates();
    startupSequence();
    waitTicks(100);
    while (1) {
        serviceStatusLeds();  ///Update status LEDs
        // Check for button combos to start tests, demo mode, or PoV easter egg
        if (!testActive && buttons[3].pressed) {
            test_cases_start(currentTest);
        } else if (!testActive && currentTest == 0 && buttons[0].pressed && buttons[2].pressed) {
            // Only allow demo mode when no test is selected/run (currentTest == 0)
            // This prevents entering an un-exitable demo mode when the device is idle with no tests executed.
            while (1) {
                demoMode();
            }
        } else if (allTestsPassed() && buttons[1].pressed && buttons[2].pressed) {
            triggerPOVEasterEgg();
        }        
        test_cases_monitor_inputs();
        __WFI();
    }
}

