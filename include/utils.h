#pragma once
#include "stm32g4xx_hal.h"

#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#define MAX(a, b) ((a > b) ? (a) : (b))
#define MIN(a, b) ((a < b) ? (a) : (b))

static inline unsigned cycle_count(void)
{
    return DWT->CYCCNT;
}
