#pragma once

/* Stack layout built in cpu/isr.asm (must match push order). */
typedef struct
{
    unsigned int gs;
    unsigned int fs;
    unsigned int es;
    unsigned int ds;
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
    unsigned int int_no;
    unsigned int err_code;
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
    unsigned int useresp;
    unsigned int ss;
} interrupt_frame_t;

typedef void (*isr_handler_fn)(interrupt_frame_t *frame);

void isr_init(void);

void isr_dispatch(interrupt_frame_t *frame);

void isr_install(unsigned char vector, isr_handler_fn handler);
