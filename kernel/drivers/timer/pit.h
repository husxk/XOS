#pragma once

#include "timer/time.h"

#define PIT_TICK_HZ 100

void pit_init(void);

time_t pit_ticks(void);
