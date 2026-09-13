/* Freestanding kernel — entered from start_kernel.asm. */

void kernel_main(void)
{
    volatile unsigned short *vga = (unsigned short *)0xb8000;
    const char *msg = "Hello from C kernel!";

    for (unsigned i = 0; msg[i]; ++i)
        vga[i] = (unsigned short)(msg[i] | 0x0f00);
}
