
#include <stdint.h>
#include <stdbool.h>
#include "ch32fun.h"
#include "pins.h"
#include "hardware.h"
#include "buttons.h"
#include "leds.h"
#include "globals.h"
#include "pins.h"

// Pin port/mask arrays/structs (define here)
const PinDef COL[5] = {
    {GPIOC, 0x01}, {GPIOC, 0x02}, {GPIOC, 0x04}, {GPIOC, 0x08}, {GPIOC, 0x10}
};
const PinDef ROW_RED   = { GPIOA, 0x02 };
const PinDef ROW_GREEN = { GPIOA, 0x04 };
const PinDef BTN[4] = {
    {GPIOD, 0x04}, {GPIOD, 0x08}, {GPIOD, 0x10}, {GPIOD, 0x20}
};

// Global variables (define here)
volatile uint32_t msTicks = 0;
volatile bool flashState = false;
volatile bool rapidFlashState = false;
volatile bool runMode = false;
volatile uint8_t currentCol = 0;

static const int statusLEDs[] = { PIN_PWR, PIN_INIT, PIN_RDY, PIN_RUN, PIN_IDLE };
static const int testCols[]   = { PIN_COL_A, PIN_COL_B, PIN_COL_C, PIN_COL_D, PIN_COL_E };
static const int inputPins[]  = { PIN_INPUT_A, PIN_INPUT_B, PIN_INPUT_C, PIN_INPUT_D };

// Timer ISR for 1ms tick, button polling, and LED scan
void TIM1_UP_IRQHandler(void) INTERRUPT_DECORATOR;
void TIM1_UP_IRQHandler(void)
{
    if (TIM1->INTFR & TIM_UIF) {
        TIM1->INTFR &= ~TIM_UIF;
        msTicks++;
        pollButtons();
        static uint32_t nextFlashAt = 250;
        if (msTicks >= nextFlashAt) {
            nextFlashAt += 250;
            flashState = !flashState;
        }
        static uint32_t nextRapidFlashAt = 125;
        if (msTicks >= nextRapidFlashAt) {
            nextRapidFlashAt += 125;
            rapidFlashState = !rapidFlashState;
        }
        scanStep();
    }
}

void setupPins(void) {
    funGpioInitAll();
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    for (int i = 0; i < 5; i++) {
        funPinMode(statusLEDs[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(statusLEDs[i], FUN_HIGH);
    }
    for (int i = 0; i < 5; i++) {
        funPinMode(testCols[i], GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
        funDigitalWrite(testCols[i], FUN_LOW);
    }
    funPinMode(PIN_ROW_R, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funPinMode(PIN_ROW_G, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
    funDigitalWrite(PIN_ROW_R, FUN_HIGH);
    funDigitalWrite(PIN_ROW_G, FUN_HIGH);
    for (int i = 0; i < 4; i++) {
        funPinMode(inputPins[i], GPIO_CNF_IN_PUPD);
        funDigitalWrite(inputPins[i], FUN_HIGH);
    }
}

void setupStatusFlasher(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
    TIM1->PSC    = 48 - 1;
    TIM1->ATRLR  = 1000 - 1;
    TIM1->SWEVGR |= TIM_UG;
    TIM1->INTFR  &= ~TIM_UIF;
    TIM1->DMAINTENR |= TIM_UIE;
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    TIM1->CTLR1 |= TIM_CEN;
}

void runStatus(bool isRun) {
    runMode = isRun;
    funDigitalWrite(PIN_RUN,  FUN_HIGH);
    funDigitalWrite(PIN_IDLE, FUN_HIGH);
}

void waitTicks(uint32_t ticks) {
    uint32_t start = msTicks;
    while ((uint32_t)(msTicks - start) < ticks) {
        __WFI();
    }
}

void scanStep(void) {
    static int8_t prev = -1;
    if (activeStates == NULL) {
        gpio_set(ROW_RED.port,   ROW_RED.mask);
        gpio_set(ROW_GREEN.port, ROW_GREEN.mask);
        return;
    }
    gpio_set(ROW_RED.port,   ROW_RED.mask);
    gpio_set(ROW_GREEN.port, ROW_GREEN.mask);
    if (prev >= 0 && prev < 5) {
        if (COL[prev].port != NULL) {
            gpio_clear(COL[prev].port, COL[prev].mask);
        }
    }
    if (currentCol >= 0 && currentCol < 5) {
        TestCaseState state = activeStates[currentCol];
        switch (state) {
            case TC_PASS:
                gpio_clear(ROW_GREEN.port, ROW_GREEN.mask);
                break;
            case TC_FAIL:
                gpio_clear(ROW_RED.port, ROW_RED.mask);
                break;
            case TC_IN_PROGRESS:
                if (flashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                else
                    gpio_clear(ROW_GREEN.port, ROW_GREEN.mask);
                break;
            case TC_WARNING:
                if (flashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                break;
            case TC_RETRY:
                if (rapidFlashState)
                    gpio_clear(ROW_RED.port, ROW_RED.mask);
                break;
            case TC_NO_RESULT:
            default:
                break;
        }
        if (COL[currentCol].port != NULL) {
            gpio_set(COL[currentCol].port, COL[currentCol].mask);
        }
    }
    prev = currentCol;
    currentCol = (currentCol + 1) % 5;
}
