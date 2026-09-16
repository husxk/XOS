#pragma once

#include "cpu/pic.h"

static inline unsigned char irq_vector(unsigned char irq)
{
    return (unsigned char)(PIC_IRQ_BASE + irq);
}

static inline void irq_ack(unsigned char irq)
{
    pic_eoi(irq);
}
