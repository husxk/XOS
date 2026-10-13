#include "timer/timer.h"

#include "drivers/timer/pit.h"
#include "kthread/kthread.h"

#if TIMER_TICK_HZ != PIT_TICK_HZ
#error TIMER_TICK_HZ must match PIT_TICK_HZ
#endif

void ktimer_init(void)
{
    pit_init();
}

tick_t ktimer_ticks(void)
{
    return pit_ticks();
}

time_t ktimer_ms(void)
{
    unsigned long long ms;

    ms = (unsigned long long)pit_ticks() * 1000u / PIT_TICK_HZ;
    return (time_t)ms;
}

void ktimer_on_tick(void)
{
    kthread_on_tick(ktimer_ticks());
}

static tick_t ktimer_ms_to_ticks(time_t ms)
{
    unsigned long long ticks;

    ticks = ((unsigned long long)ms * (unsigned long long)PIT_TICK_HZ + 999u) / 1000u;
    if (ticks == 0u && ms != 0)
        ticks = 1u;

    return (tick_t)ticks;
}

void ksleep_ms(time_t ms)
{
    tick_t wake_at;

    if (ms == 0)
        return;

    wake_at = ktimer_ticks() + ktimer_ms_to_ticks(ms);
    kthread_sleep_interruptible_until(wake_at);
}
