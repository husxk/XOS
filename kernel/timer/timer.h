#pragma once

#include "timer/tick.h"
#include "timer/time.h"

#define TIMER_TICK_HZ 100

void ktimer_init(void);

tick_t ktimer_ticks(void);

time_t ktimer_ms(void);

void ktimer_on_tick(void);
