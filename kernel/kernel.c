/* Freestanding kernel — entered from start_kernel.asm. */

#include "log/kprint.h"

static const char msg[] = "Hello from kernel!";

static void kernel_init(void)
{
    kprint_init();
}

void kernel_main(void)
{
    kernel_init();

    kprint(msg);
    kprint("\n");
    kprint_hexdump(msg, sizeof(msg) - 1);
}
