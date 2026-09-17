#include "drivers/timer/pit.h"

#include "cpu/io.h"
#include "cpu/irq.h"
#include "cpu/isr.h"
#include "cpu/pic.h"

#define PIT_IRQ            0
#define PIT_BASE_HZ        1193182
#define PIT_CHANNEL0_DATA  0x40
#define PIT_COMMAND        0x43

/* 8254 control word (port 0x43): 7–6 channel | 5–4 access | 3–1 mode | 0 BCD. */
#define PIT_CMD_CH0_MODE3 0b00110110 /* 00 ch0 | 11 lobyte+hibyte | 011 mode 3 | 0 binary */

static volatile time_t pit_tick_count;

static void pit_isr(interrupt_frame_t *frame)
{
    (void)frame;

    pit_tick_count++;
    irq_ack(PIT_IRQ);
}

static void pit_set_frequency(unsigned int hz)
{
    unsigned int divisor;

    if (hz == 0)
        return;

    divisor = PIT_BASE_HZ / hz;
    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xffff)
        divisor = 0xffff;

    io_outb(PIT_COMMAND, PIT_CMD_CH0_MODE3);
    io_outb(PIT_CHANNEL0_DATA, (unsigned char)(divisor & 0xff));
    io_outb(PIT_CHANNEL0_DATA, (unsigned char)((divisor >> 8) & 0xff));
}

void pit_init(void)
{
    pit_tick_count = 0;
    pit_set_frequency(PIT_TICK_HZ);
    isr_install_irq(PIT_IRQ, pit_isr);
    pic_unmask(PIT_IRQ);
}

time_t pit_ticks(void)
{
    return pit_tick_count;
}
