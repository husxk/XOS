/* Freestanding kernel — entered from start_kernel.asm. */

#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/pic.h"
#include "drivers/keyboard/keyboard.h"
#include "log/kprint.h"
#include "mem/phys_map.h"
#include "mem/phys_mem.h"
#include "timer/timer.h"

#define TICK_REPORT_MS (10u * 1000u)

static const char msg[] = "Hello from kernel!";

static void kernel_init(void)
{
    kprint_init();
    phys_map_init();
    phys_mem_init();

    idt_init();
    isr_init();
    pic_init();
    ktimer_init();
    keyboard_init();

    cpu_enable_interrupts();
}

void kernel_main(void)
{
    kernel_init();

    kputs(msg);
    kputs("\n");
    phys_map_print();
    kprint_hexdump(msg, sizeof(msg) - 1);
    phys_mem_print_stats();

    {
        void *page_a = phys_mem_alloc_page();
        void *page_b = phys_mem_alloc_page();

        if (page_a != 0 && page_b != 0)
        {
            kprint("phys_mem test alloc 0x%x 0x%x\n",
                   (unsigned int)(unsigned long)page_a,
                   (unsigned int)(unsigned long)page_b);

            phys_mem_free_page(page_a);
            phys_mem_free_page(page_b);
            phys_mem_free_page(page_a);
        }
        else
            kprint("phys_mem test alloc failed\n");
    }

    phys_mem_print_stats();

    for (;;)
    {
        time_t ticks;

        ksleep_ms((time_t)TICK_REPORT_MS);
        ticks = ktimer_ticks();
        kprint("ticks %u\n", (unsigned int)ticks);
    }
}
