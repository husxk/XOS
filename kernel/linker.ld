ENTRY(_start)

SECTIONS
{
    . = 0x1000;

    .multiboot_header : {
        KEEP(*(.multiboot_header))
    }

    .text : {
        *(.text.entry)
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(COMMON)
        *(.bss*)
    }
}
