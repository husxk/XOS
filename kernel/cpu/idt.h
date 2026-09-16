#pragma once

#define IDT_ENTRIES 256

#define KERNEL_CODE_SELECTOR 0x08

/* 32-bit interrupt gate, present, DPL 0 */
#define IDT_GATE_INTERRUPT 0x8E

typedef struct
{
    unsigned short offset_low;
    unsigned short selector;
    unsigned char reserved;
    unsigned char flags;
    unsigned short offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed)) idtr_t;

void idt_init(void);

void idt_set_gate(unsigned char vector, void (*handler)(void), unsigned short selector,
                  unsigned char flags);

void idt_load(void);
