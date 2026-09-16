#include "cpu/idt.h"

#include "mem/kmem.h"

/*
 * Alignment of the in-memory IDT array.
 *
 * The CPU only needs the table base from LIDT to address 8-byte gates; aligning
 * the table to sizeof(idt_entry_t) (8) is sufficient. Some projects use 512
 * as an arbitrary power-of-two — that habit is often confused with the 512-byte
 * boot sector or with IDT_ENTRIES being 256, but the SDM does not require it.
 */
#define IDT_TABLE_ALIGNMENT 8

static idt_entry_t idt[IDT_ENTRIES] __attribute__((aligned(IDT_TABLE_ALIGNMENT)));
static idtr_t idtr;

void idt_set_gate(unsigned char vector, void (*handler)(void), unsigned short selector,
                  unsigned char flags)
{
    unsigned long address;

    if (vector >= IDT_ENTRIES || handler == 0)
        return;

    address = (unsigned long)handler;

    idt[vector].offset_low = (unsigned short)(address & 0xffff);
    idt[vector].selector = selector;
    idt[vector].reserved = 0;
    idt[vector].flags = flags;
    idt[vector].offset_high = (unsigned short)((address >> 16) & 0xffff);
}

void idt_load(void)
{
    idtr.limit = (unsigned short)(sizeof(idt) - 1);
    idtr.base = (unsigned int)(unsigned long)idt;

    __asm__ volatile("lidt %0" : : "m"(idtr) : "memory");
}

void idt_init(void)
{
    kmemset(idt, 0, sizeof(idt));
}
