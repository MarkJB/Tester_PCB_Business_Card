

#include "pins.h"
#include "buttons.h"
#include <stdint.h>
#include <stdbool.h>
#include "ch32fun.h"
#include "hardware.h"
#include "globals.h"
#include "pins.h"

volatile ButtonState buttons[4];
static uint8_t lastButtonSample = 0xFF;
static uint32_t initLedOffAt = 0;

void pollButtons(void) {
    uint8_t sample = 0;
    for (int i = 0; i < 4; i++) {
        if (!gpio_get(BTN[i].port, BTN[i].mask)) {
            sample |= (1 << i);
        }
    }

    uint8_t changed = sample ^ lastButtonSample;
    if (changed) {
        for (int i = 0; i < 4; i++) {
            if ((changed & (1 << i)) && (sample & (1 << i))) {
                buttons[i].pressed = true;
                buttons[i].justPressed = true;
                buttons[i].lastPressTime = msTicks;
                buttons[i].pressCount++;
                gpio_clear(GPIOC, BIT(6));
                initLedOffAt = msTicks + 50;
            } else if (changed & (1 << i)) {
                buttons[i].pressed = false;
                buttons[i].lastReleaseTime = msTicks;
            }
        }
    }

    if (initLedOffAt && msTicks >= initLedOffAt) {
        gpio_set(GPIOC, BIT(6));
        initLedOffAt = 0;
    }

    lastButtonSample = sample;
}
