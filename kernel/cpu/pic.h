#pragma once

#define PIC_IRQ_BASE 32
#define PIC_IRQ_COUNT 16

void pic_init(void);

void pic_mask_all(void);

void pic_unmask(unsigned char irq);

void pic_eoi(unsigned char irq);

void cpu_enable_interrupts(void);
