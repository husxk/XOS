#pragma once

#define PHYS_MEM_PAGE_SIZE 4096u

void phys_mem_init(void);

/* Returns a physical address (identity-mapped). NULL only on OOM (not phys 0). */
void *phys_mem_alloc_page(void);

void phys_mem_free_page(void *page);

void phys_mem_print_stats(void);
