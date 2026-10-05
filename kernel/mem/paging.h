#pragma once

/* 32-bit x86 paging (non-PAE). Boot maps kernel/PMM/VGA only; paging_init() then paging_enable(). */

void paging_init(void);

void paging_enable(void);

/* One 4 KiB supervisor mapping; virt and phys must be page-aligned. Returns 1 on success. */
int paging_map_page(unsigned long virt, unsigned long phys);
