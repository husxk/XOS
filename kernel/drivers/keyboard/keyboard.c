#include "drivers/keyboard/keyboard.h"

#include "cpu/io.h"
#include "cpu/irq.h"
#include "cpu/isr.h"
#include "cpu/pic.h"
#include "log/kprint.h"

#define KBD_IRQ       1
#define KBD_DATA_PORT 0x60

static void keyboard_isr(interrupt_frame_t *frame)
{
    unsigned char scancode;

    (void)frame;

    scancode = io_inb(KBD_DATA_PORT);
    irq_ack(KBD_IRQ);

    kprint("key ");
    kprint_hex32(scancode);
    kprint("\n");
}

void keyboard_init(void)
{
    isr_install_irq(KBD_IRQ, keyboard_isr);
    pic_unmask(KBD_IRQ);
}
