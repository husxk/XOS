; Linked at 0x1000 — boot.asm jumps here after loading disk image.
bits 32

; Must be first in the linked image — boot jumps to physical 0x1000 (= start of .text in kernel.bin).
section .text.entry

global _start
extern kernel_main

_start:
    call kernel_main
.hang:
    hlt
    jmp .hang
