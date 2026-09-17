#pragma once

#include "timer/time.h"

#define TIMER_TICK_HZ 100

void ktimer_init(void);

time_t ktimer_ticks(void);

time_t ktimer_ms(void);
