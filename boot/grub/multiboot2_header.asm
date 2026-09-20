; Multiboot2 header — linked into kernel.elf (see grub.cfg).
bits 32

section .multiboot_header
align 8
header_start:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture: i386 protected mode
    dd header_end - header_start
    dd -(0xe85250d6 + 0 + (header_end - header_start))

    ; End tag (required).
    align 8
    dw 0                         ; type: end
    dw 0                         ; flags
    dd 8                         ; size
header_end:
