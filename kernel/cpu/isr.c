#include "cpu/isr.h"

#include "cpu/idt.h"
#include "cpu/irq.h"
#include "cpu/pic.h"
#include "log/kprint.h"

extern void (*isr_stub_table[])(void);

static isr_handler_fn handlers[IDT_ENTRIES];

typedef struct
{
    const char *abbr;
    const char *full;
} exception_info_t;

#define CPU_EXCEPTION_COUNT 32

static const exception_info_t cpu_exception[CPU_EXCEPTION_COUNT] =
{
    { "DE",  "Divide Error" },
    { "DB",  "Debug" },
    { "NMI", "Non-Maskable Interrupt" },
    { "BP",  "Breakpoint" },
    { "OF",  "Overflow" },
    { "BR",  "BOUND Range Exceeded" },
    { "UD",  "Invalid Opcode" },
    { "NM",  "Device Not Available" },
    { "DF",  "Double Fault" },
    { "CSO", "Coprocessor Segment Overrun" },
    { "TS",  "Invalid TSS" },
    { "NP",  "Segment Not Present" },
    { "SS",  "Stack-Segment Fault" },
    { "GP",  "General Protection" },
    { "PF",  "Page Fault" },
    { "RS",  "Reserved" },
    { "MF",  "x87 FPU Error" },
    { "AC",  "Alignment Check" },
    { "MC",  "Machine Check" },
    { "XM",  "SIMD Floating-Point" },
    { "VE",  "Virtualization" },
    { "CP",  "Control Protection" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
    { "RS",  "Reserved" },
};

static void print_exception(unsigned int vector)
{
    if (vector < CPU_EXCEPTION_COUNT)
    {
        kprint("%s (%s)", cpu_exception[vector].full, cpu_exception[vector].abbr);
        return;
    }

    if (vector >= PIC_IRQ_BASE && vector < PIC_IRQ_BASE + PIC_IRQ_COUNT)
    {
        /* Hex keeps the panic dump uniform; %u would read better for 0-15. */
        kprint("Hardware IRQ 0x%08x", vector - PIC_IRQ_BASE);
        return;
    }

    kputs("Interrupt");
}

static void kernel_panic(interrupt_frame_t *frame)
{
    unsigned int cr2;

    kputs("\n*** KERNEL PANIC ***\n");

    kprint("vector 0x%08x ", frame->int_no);

    print_exception(frame->int_no);
    kputs("\n");

    kprint("err_code 0x%08x\n", frame->err_code);
    kprint("eip 0x%08x cs 0x%08x eflags 0x%08x\n", frame->eip, frame->cs, frame->eflags);
    kprint("eax 0x%08x ebx 0x%08x ecx 0x%08x edx 0x%08x\n",
           frame->eax, frame->ebx, frame->ecx, frame->edx);

    if (frame->int_no == 14)
    {
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

        kprint("cr2 0x%08x\n", cr2);
    }

    for (;;)
        __asm__ volatile("cli; hlt" ::: "memory");
}

static void isr_default_handler(interrupt_frame_t *frame)
{
    unsigned int vector;

    vector = frame->int_no;
    if (vector >= PIC_IRQ_BASE && vector < PIC_IRQ_BASE + PIC_IRQ_COUNT)
        pic_eoi((unsigned char)(vector - PIC_IRQ_BASE));

    kernel_panic(frame);
}

void isr_install(unsigned char vector, isr_handler_fn handler)
{
    if (vector >= IDT_ENTRIES || handler == 0)
        return;

    handlers[vector] = handler;
}

void isr_install_irq(unsigned char irq, isr_handler_fn handler)
{
    if (irq >= PIC_IRQ_COUNT || handler == 0)
        return;

    isr_install(irq_vector(irq), handler);
}

void isr_dispatch(interrupt_frame_t *frame)
{
    unsigned int vector;

    if (frame == 0)
        return;

    vector = frame->int_no;
    if (vector >= IDT_ENTRIES)
        vector = 0;

    handlers[vector](frame);
}

void isr_init(void)
{
    unsigned int i;

    for (i = 0; i < IDT_ENTRIES; i++)
    {
        handlers[i] = isr_default_handler;
        idt_set_gate((unsigned char)i, isr_stub_table[i], KERNEL_CODE_SELECTOR, IDT_GATE_INTERRUPT);
    }

    idt_load();
}
