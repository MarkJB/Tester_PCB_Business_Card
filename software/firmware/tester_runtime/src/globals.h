#ifndef GLOBALS_H
#define GLOBALS_H
#include <stdint.h>
#include <stdbool.h>

extern volatile uint32_t msTicks;
extern volatile bool flashState;
extern volatile bool rapidFlashState;
extern volatile bool runMode;
extern volatile uint8_t currentCol;

#endif // GLOBALS_H
