/* Freestanding kernel — entered from start_kernel.asm. */

#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/pic.h"
#include "drivers/keyboard/keyboard.h"
#include "log/kprint.h"
#include "mem/kmem.h"
#include "mem/paging.h"
#include "mem/phys_map.h"
#include "mem/phys_map_kernel.h"
#include "mem/phys_mem.h"
#include "mem/phys_mem_kernel.h"
#include "mem/phys_mem_print.h"
#include "timer/timer.h"
#include "drivers/vga/vga.h"
#include "kthread/kthread.h"

static const char msg[] = "Hello from kernel!\n";

static void heap_smoke_test(void)
{
    void *a;
    void *b;
    unsigned char *bytes;

    a = kmalloc(64);
    if (a == 0)
    {
        kprint("heap test: alloc(64) failed\n");
        return;
    }

    bytes = (unsigned char *)a;
    bytes[0] = 0xab;
    bytes[63] = 0xcd;

    b = kmalloc(128);
    if (b == 0)
    {
        kprint("heap test: alloc(128) failed\n");
        kfree(a);
        return;
    }

    kprint("heap test: a=0x%x b=0x%x\n",
           (unsigned int)(unsigned long)a, (unsigned int)(unsigned long)b);

    kfree(a);
    kfree(b);

    a = kmalloc(32);
    if (a == 0)
        kprint("heap test: alloc(32) after free failed\n");
    else
    {
        kprint("heap test: reuse 0x%x\n", (unsigned int)(unsigned long)a);
        kfree(a);
    }

    kprint("heap test: done\n");
}

static void on_key(char c)
{
    vga_putchar(c);
}

struct demo_kthread_params
{
    int id;
    time_t delay_ms;
};

static struct demo_kthread_params demo_kthread_a = { 0, (time_t)1000 };
static struct demo_kthread_params demo_kthread_b = { 1, (time_t)2000 };

static void demo_kthread(void *arg)
{
    struct demo_kthread_params *p = arg;

    for (;;)
    {
        tick_t ticks_before = ktimer_ticks();

        kprint("kthread demo %d: timeout %u ms, ticks %u\n",
               p->id, (unsigned int)p->delay_ms, (unsigned int)ticks_before);

        ksleep_ms(p->delay_ms);

        kprint("kthread demo %d: done, ticks %u\n",
               p->id, (unsigned int)ktimer_ticks());
    }
}

static void kernel_init(void)
{
    kprint_init();
    phys_map_kernel_init();
    phys_mem_kernel_init();
    paging_init();

    idt_init();
    isr_init();
    paging_enable();

    kheap_init();
    kthread_init();

    pic_init();
    ktimer_init();

    keyboard_init();
    keyboard_set_handler(on_key);

    cpu_enable_interrupts();
}

void kernel_main(void)
{
    kernel_init();

    kputs(msg);
    phys_map_print();
    phys_mem_print_stats();

    {
        void *block = phys_mem_alloc_pages(4);

        if (block != 0)
        {
            kprint("phys_mem test alloc_pages(4) 0x%x\n",
                   (unsigned int)(unsigned long)block);
            phys_mem_free_pages(block, 4);
        }
        else
            kprint("phys_mem test alloc_pages failed\n");
    }

    phys_mem_print_stats();
    heap_smoke_test();

    if (kthread_create(demo_kthread, &demo_kthread_a) == KTHREAD_ID_NONE)
        kprint("kthread demo: create 0 failed\n");

    if (kthread_create(demo_kthread, &demo_kthread_b) == KTHREAD_ID_NONE)
        kprint("kthread demo: create 1 failed\n");

    kthread_run();
}
