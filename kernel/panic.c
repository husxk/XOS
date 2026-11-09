#include "panic.h"

#include "log/kprint.h"

void panic_begin(void)
{
    /*
     * Interrupts go down first, before anything is printed.
     *
     * A timer tick between here and panic_halt() would schedule
     * another thread on top of a kernel already known to be broken,
     * and an ISR printing its own output would interleave with the
     * panic message.
     *
     * The memory clobber keeps the compiler from
     * moving loads or stores across this line, so everything below
     * really does run with interrupts off.
     */
    __asm__ volatile("cli" ::: "memory");
    kputs("\n*** KERNEL PANIC ***\n");
}

void panic_halt(void)
{
    /*
     * Disable interrupts here too, so panic_halt() is self-sufficient
     * even if it is ever reached without panic_begin() first. With
     * interrupts on, hlt would wake on the next one and spin instead of
     * parking the CPU for good.
     *
     * Neither asm needs a memory clobber: by this point nothing is left
     * to print or order, and hlt touches neither memory nor registers.
     */
    __asm__ volatile("cli");
    for (;;)
        __asm__ volatile("hlt");
}

void kpanic(const char *file, int line, const char *msg)
{
    panic_begin();
    kprint("%s:%d: assertion failed: %s\n", file, line, msg);
    panic_halt();
}
