#pragma once

#include "mem/mem_layout.h"
#include "mem/phys_mem_err.h"

#define PHYS_MEM_PAGE_SIZE XOS_PAGE_SIZE

int phys_mem_init(void);

void phys_mem_stats(unsigned long *frame_total, unsigned long *free_count,
                    unsigned long *bitmap_size_bytes);

/* Page-aligned [base, base + page_span) of the frame bitmap; (0, 0) if unset. */
void phys_mem_bitmap_storage_span(unsigned long *base, unsigned long *page_span);

/* Physical address (identity-mapped). NULL only on OOM (not phys 0). */
void *phys_mem_alloc_pages(unsigned long page_count);

void *phys_mem_alloc_page(void);

int phys_mem_free_pages(void *base, unsigned long page_count);

int phys_mem_free_page(void *page);
