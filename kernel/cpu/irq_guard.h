#pragma once

static inline void cpu_irq_disable(void)
{
    __asm__ volatile("cli" ::: "memory");
}

static inline void cpu_irq_enable(void)
{
    __asm__ volatile("sti" ::: "memory");
}

/* Wait for an interrupt; caller usually enables IRQs first (see cpu_irq_enable). */
static inline void cpu_halt(void)
{
    __asm__ volatile("hlt" ::: "memory");
}
