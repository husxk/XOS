/* Freestanding kernel — entered from start_kernel.asm. */

#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/pic.h"
#include "log/kprint.h"

static const char msg[] = "Hello from kernel!";

static void kernel_init(void)
{
    kprint_init();

    idt_init();
    isr_init();
    pic_init();

    cpu_enable_interrupts();
}

void kernel_main(void)
{
    kernel_init();

    kputs(msg);
    kputs("\n");
    kprint_hexdump(msg, sizeof(msg) - 1);
}
