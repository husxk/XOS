; Multiboot2 header — linked into kernel.elf (see grub.cfg).
; Tag layout: u16 type, u16 flags, u32 size (Multiboot2 spec / GRUB).
;
; ref: https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#OS-image-format

bits 32

%define MB2_HEADER_TAG_END                 0
%define MB2_HEADER_TAG_INFORMATION_REQUEST 1
%define MB2_INFO_TAG_MMAP                  6

section .multiboot_header
align 8
header_start:
    dd 0xe85250d6                ; magic
    dd 0                         ; architecture: i386 protected mode
    dd header_end - header_start
    dd -(0xe85250d6 + 0 + (header_end - header_start))

    ; Ask the bootloader for a physical memory map (info tag type 6).
    align 8
mmap_request_tag:
    dw MB2_HEADER_TAG_INFORMATION_REQUEST
    dw 0
    dd mmap_request_end - mmap_request_tag
    dd MB2_INFO_TAG_MMAP
    dd 0                         ; end of request list
mmap_request_end:

    align 8
    dw MB2_HEADER_TAG_END
    dw 0
    dd 8
header_end:
