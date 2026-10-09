#pragma once

#include "mem/mem_layout.h"
#include "mem/phys_mem_err.h"

#define PHYS_MEM_PAGE_SIZE XOS_PAGE_SIZE

#define PHYS_MEM_RESERVED_MAX 8

struct phys_mem_reserved_range
{
    unsigned long base;
    unsigned long length;
};

struct phys_mem_init_params
{
    /* First AVAIL region fit for the frame bitmap at or above this physical address. */
    unsigned long bitmap_min_phys;

    const struct phys_mem_reserved_range *reserved;
    unsigned int reserved_count;
};

#ifdef __cplusplus
extern "C" {
#endif

int phys_mem_init_from_params(const struct phys_mem_init_params *params);

void phys_mem_stats(unsigned long *frame_total, unsigned long *free_count,
                    unsigned long *bitmap_size_bytes);

void phys_mem_bitmap_storage_span(unsigned long *base, unsigned long *page_span);

void *phys_mem_alloc_pages(unsigned long page_count);

void *phys_mem_alloc_page(void);

int phys_mem_free_pages(void *base, unsigned long page_count);

int phys_mem_free_page(void *page);

#ifdef __cplusplus
}
#endif
