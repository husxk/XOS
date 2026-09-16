; CPU interrupt stubs (vectors 0–255)
; Linked as elf32;
bits 32

extern isr_dispatch

%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0          ; dummy err_code (cpu did not push one)
    push dword %1         ; vector number for isr_dispatch
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1         ; vector (err_code already on stack from cpu)
    jmp isr_common_stub
%endmacro

; Same flat data selector as boot/boot.asm (GDT_data).
%define KERNEL_DATA_SELECTOR 0x10

isr_common_stub:

    ; Build interrupt_frame on stack: ISR_* already pushed err_code + vector;
    ; then pusha and segment saves; cpu-pushed eip/cs/eflags sit below that.
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, KERNEL_DATA_SELECTOR
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; pass pointer to interrupt_frame (esp) as isr_dispatch's argument.
    push esp

    call isr_dispatch

    ; drop pushed esp (pointer to interrupt_frame), not pusha/segments.
    add esp, 4

    pop gs
    pop fs
    pop es
    pop ds

    popa

    ; Drop stub-pushed err_code + vector (see ISR_* macros); iret needs cpu frame on top.
    add esp, 8

    iret

%assign i 0
%rep 256
    ; Error code from CPU: classic set + #CP (21, CET).
    %if i == 8 || i == 10 || i == 11 || i == 12 || i == 13 || i == 14 || i == 17 || i == 21
        ISR_ERRCODE i
    %else
        ISR_NOERRCODE i
    %endif
    %assign i i + 1
%endrep

section .rodata
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dd isr %+ i
%assign i i + 1
%endrep
