#ifndef BIT
#define BIT(x) (1U << (x))
#endif
#ifndef PINS_H
#define PINS_H
#include "ch32fun.h"

// Status LEDs (active LOW)
#define PIN_PWR   PC5
#define PIN_INIT  PC6
#define PIN_RDY   PC7
#define PIN_RUN   PD0
#define PIN_IDLE  PD6

// Test Case Columns (TC1–TC5), active HIGH
#define PIN_COL_A PC0
#define PIN_COL_B PC1
#define PIN_COL_C PC2
#define PIN_COL_D PC3
#define PIN_COL_E PC4

// Test Case Rows (active LOW)
#define PIN_ROW_R PA1  // Red (fail)
#define PIN_ROW_G PA2  // Green (pass)

// Input buttons (assumed active LOW with pull-ups)
#define PIN_INPUT_A  PD2
#define PIN_INPUT_B  PD3
#define PIN_INPUT_C  PD4
#define PIN_INPUT_D  PD5


typedef struct { GPIO_TypeDef* port; int mask; } PinDef;

extern const PinDef COL[5];
extern const PinDef ROW_RED;
extern const PinDef ROW_GREEN;
extern const PinDef BTN[4];

static inline void gpio_set(GPIO_TypeDef* p, int m)   { p->BSHR = m; }
static inline void gpio_clear(GPIO_TypeDef* p, int m) { p->BCR  = m; }
static inline uint8_t gpio_get(GPIO_TypeDef* p, int m) {
    return (p->INDR & m) ? 1U : 0U;
}

#endif // PINS_H
