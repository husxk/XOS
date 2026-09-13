/* Freestanding kernel — entered from start_kernel.asm. */

#include "drivers/vga/vga.h"
#include "log/kprint.h"

static const char msg[] = "Hello from C kernel!";


void kernel_main(void)
{
    vga_init();

    kprint(msg);
    kprint("\n");
    kprint_hexdump(msg, sizeof(msg) - 1);
}
