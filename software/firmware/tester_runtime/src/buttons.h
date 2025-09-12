#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t lastPressTime;
    uint32_t lastReleaseTime;
    uint16_t pressCount;
    bool     pressed;
    bool     justPressed;
} ButtonState;

extern volatile ButtonState buttons[4];
void pollButtons(void);

#endif // BUTTONS_H
