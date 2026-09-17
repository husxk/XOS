#include "timer/timer.h"

#include "drivers/timer/pit.h"

#if TIMER_TICK_HZ != PIT_TICK_HZ
#error TIMER_TICK_HZ must match PIT_TICK_HZ
#endif

void ktimer_init(void)
{
    pit_init();
}

time_t ktimer_ticks(void)
{
    return pit_ticks();
}

time_t ktimer_ms(void)
{
    unsigned long long ms;

    ms = (unsigned long long)pit_ticks() * 1000u / PIT_TICK_HZ;
    return (time_t)ms;
}

void ksleep_ms(time_t ms)
{
    time_t start;

    if (ms == 0)
        return;

    start = ktimer_ms();
    while ((time_t)(ktimer_ms() - start) < ms)
        __asm__ volatile("hlt");
}
