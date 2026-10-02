#pragma once

/* 32-bit x86 paging (non-PAE). Boot maps kernel/PMM/VGA only; paging_init() then paging_enable(). */

void paging_init(void);

void paging_enable(void);
