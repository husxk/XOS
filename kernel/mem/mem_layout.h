#pragma once

/* i686 non-PAE 4 KiB pages — shared by PMM and paging. */
#define XOS_PAGE_SIZE 4096u

/*
 * Kernel linked/loaded physical base. Keep in sync with kernel/linker.ld and
 * boot/bios/boot-bios.asm (KERNEL_LOCATION).
 */
#define KERNEL_LOAD_PHYS 0x1000u

/*
 * Kernel VA used only to memset() the next L2 frame before it is linked from
 * a PDE. Retargeted with paging_map_page(); not the PT's final identity VA.
 * Page-aligned; must stay below KERNEL_HEAP_BASE.
 */
#define PAGING_PT_STAGING_VA 0x007ff000u

/*
 * Kernel heap virtual range.
 * Must be page-aligned.
 */
#define KERNEL_HEAP_BASE 0x00800000u
#define KERNEL_HEAP_PAGE_COUNT 64u
#define KERNEL_HEAP_SIZE (KERNEL_HEAP_PAGE_COUNT * XOS_PAGE_SIZE)
