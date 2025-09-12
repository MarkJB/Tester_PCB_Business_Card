#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>
#include <stdbool.h>
#include "ch32fun.h"
#include "test_cases.h"

void setupPins(void);
void setupStatusFlasher(void);
void runStatus(bool isRun);
void waitTicks(uint32_t ticks);
void scanStep(void);

#endif // HARDWARE_H
