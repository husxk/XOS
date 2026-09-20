; Kernel entry — GRUB Multiboot2 (see boot/grub/).
bits 32

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .rodata
align 8
gdt_null:
    dq 0

gdt_code:                      ; flat 32-bit code, base 0, limit 4G
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0b10011010
    db 0b11001111
    db 0x00

gdt_data:                      ; flat 32-bit data
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0b10010010
    db 0b11001111
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_null - 1
    dd gdt_null

%define KERNEL_CODE_SEG 0x08   ; matches kernel/cpu/idt.h KERNEL_CODE_SELECTOR
%define KERNEL_DATA_SEG 0x10   ; matches cpu/isr.asm KERNEL_DATA_SELECTOR

section .text.entry

global _start
extern kernel_main

%define MULTIBOOT2_BOOT_MAGIC 0x36d76289

global multiboot2_info
multiboot2_info:
    dd 0

_start:
    cli

    cmp eax, MULTIBOOT2_BOOT_MAGIC
    jne .load_gdt
    mov [multiboot2_info], ebx

.load_gdt:
    ; GRUB's GDT selectors differ from our IDT/ISR stubs — install our flat map.
    lgdt [gdt_descriptor]
    jmp KERNEL_CODE_SEG:.flush_cs

.flush_cs:
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    mov esp, stack_top
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
