#include "cpu/pic.h"

#include "cpu/io.h"

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21

#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1

#define PIC_ICW1_INIT   0x11 /* ICW1_INIT + ICW4 present */
#define PIC_ICW4_8086   0x01
#define PIC_EOI         0x20

/* ICW3: master IRQ2 wired to slave (PC/AT cascade). */
#define PIC_CASCADE_IRQ              2
#define PIC_ICW3_MASTER_CASCADE      (1u << PIC_CASCADE_IRQ)
#define PIC_ICW3_SLAVE_CASCADE_ID    PIC_CASCADE_IRQ

static void pic_wait(void)
{
    __asm__ volatile("nop");
    __asm__ volatile("nop");
}

void pic_mask_all(void)
{
    io_outb(PIC_MASTER_DATA, 0xff);
    io_outb(PIC_SLAVE_DATA, 0xff);
}

void pic_unmask(unsigned char irq)
{
    unsigned short port;
    unsigned char mask;

    if (irq >= PIC_IRQ_COUNT)
        return;

    port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
    irq  = (irq < 8) ? irq : (unsigned char)(irq - 8);

    mask = io_inb(port);
    mask &= (unsigned char)~(1u << irq);
    io_outb(port, mask);
}

void pic_eoi(unsigned char irq)
{
    if (irq >= PIC_IRQ_COUNT)
        return;

    if (irq >= 8)
        io_outb(PIC_SLAVE_CMD, PIC_EOI);

    io_outb(PIC_MASTER_CMD, PIC_EOI);
}

void pic_init(void)
{
    io_outb(PIC_MASTER_CMD, PIC_ICW1_INIT);
    pic_wait();
    io_outb(PIC_SLAVE_CMD, PIC_ICW1_INIT);
    pic_wait();

    io_outb(PIC_MASTER_DATA, PIC_IRQ_BASE);
    pic_wait();
    io_outb(PIC_SLAVE_DATA, (unsigned char)(PIC_IRQ_BASE + 8));
    pic_wait();

    io_outb(PIC_MASTER_DATA, PIC_ICW3_MASTER_CASCADE);
    pic_wait();
    io_outb(PIC_SLAVE_DATA, PIC_ICW3_SLAVE_CASCADE_ID);
    pic_wait();

    io_outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    pic_wait();
    io_outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    pic_wait();

    pic_mask_all();
}

void cpu_enable_interrupts(void)
{
    __asm__ volatile("sti");
}
