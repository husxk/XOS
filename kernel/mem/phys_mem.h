#pragma once

#define PHYS_MEM_PAGE_SIZE 4096u

void phys_mem_init(void);

/* Physical address (identity-mapped). NULL only on OOM (not phys 0). */
void *phys_mem_alloc_pages(unsigned long page_count);

void *phys_mem_alloc_page(void);

void phys_mem_free_pages(void *base, unsigned long page_count);

void phys_mem_free_page(void *page);

void phys_mem_print_stats(void);
