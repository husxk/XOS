#pragma once

#include "timer/tick.h"

#define PIT_TICK_HZ 100

void pit_init(void);

tick_t pit_ticks(void);
