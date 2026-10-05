#include "mem/heap.h"

#include "log/kprint.h"
#include "mem/mem_layout.h"
#include "mem/paging.h"
#include "mem/phys_mem.h"

#define HEAP_ALLOC_ALIGN 8u

/*
 * Arena layout: [heap_begin, heap_limit) is a contiguous chain of blocks;
 * advance with (uint8_t *)block + block->size. Free blocks also hang off
 * heap_free_list via next_free (only valid when free != 0).
 */
struct heap_block
{
    unsigned long size; /* entire block including this header */
    unsigned char free;
    unsigned char _pad[3];
    struct heap_block *next_free;
};

#define HEAP_BLOCK_HEADER_SIZE                                              \
    (((unsigned long)sizeof(struct heap_block) + (HEAP_ALLOC_ALIGN - 1u)) & \
     ~(unsigned long)(HEAP_ALLOC_ALIGN - 1u))

#define HEAP_MIN_BLOCK_SIZE (HEAP_BLOCK_HEADER_SIZE + HEAP_ALLOC_ALIGN)

static unsigned char *heap_begin;
static unsigned char *heap_limit;
static struct heap_block *heap_free_list;

static unsigned long heap_align_up(unsigned long n)
{
    return (n + (HEAP_ALLOC_ALIGN - 1u)) & ~(HEAP_ALLOC_ALIGN - 1u);
}

/* Remove block from the free list; used after alloc and when coalescing a neighbor. */
static void heap_unlink_free(struct heap_block *block)
{
    struct heap_block *prev;
    struct heap_block *cur;

    for (prev = 0, cur = heap_free_list; cur != 0;
         prev = cur, cur = cur->next_free)
    {
        if (cur != block)
            continue;

        if (prev != 0)
            prev->next_free = block->next_free;
        else
            heap_free_list = block->next_free;

        return;
    }
}

static struct heap_block *heap_block_at(unsigned char *addr)
{
    return (struct heap_block *)(void *)addr;
}

static struct heap_block *heap_next_physical(struct heap_block *block)
{
    unsigned char *next;

    next = (unsigned char *)block + block->size;
    if (next >= heap_limit)
        return 0;

    return heap_block_at(next);
}

static struct heap_block *heap_prev_physical(struct heap_block *block)
{
    struct heap_block *walk;

    if ((unsigned char *)block <= heap_begin)
        return 0;

    walk = heap_block_at(heap_begin);
    while ((unsigned char *)walk + walk->size < (unsigned char *)block)
        walk = heap_next_physical(walk);

    if ((unsigned char *)walk + walk->size != (unsigned char *)block)
        return 0;

    return walk;
}

/* Merge with physically adjacent free neighbors; block must already be on the free list. */
static void heap_coalesce(struct heap_block *block)
{
    struct heap_block *next;
    struct heap_block *prev;

    next = heap_next_physical(block);
    if (next != 0 && next->free)
    {
        heap_unlink_free(next);
        block->size += next->size;
    }

    prev = heap_prev_physical(block);
    if (prev != 0 && prev->free)
    {
        heap_unlink_free(block);
        prev->size += block->size;
    }
}

static int heap_map_region(void)
{
    unsigned long i;

    for (i = 0; i < KERNEL_HEAP_PAGE_COUNT; i++)
    {
        unsigned long virt = KERNEL_HEAP_BASE + i * XOS_PAGE_SIZE;
        void *phys = phys_mem_alloc_page();

        if (phys == 0)
        {
            kprint("heap: phys page alloc failed at index %u\n", (unsigned int)i);
            return 0;
        }

        if (!paging_map_page(virt, (unsigned long)phys))
        {
            kprint("heap: map failed virt 0x%x phys 0x%x\n",
                   (unsigned int)virt, (unsigned int)(unsigned long)phys);
            phys_mem_free_pages(phys, 1);
            return 0;
        }
    }

    return 1;
}

void heap_init(void)
{
    struct heap_block *initial;

    heap_begin = (unsigned char *)(unsigned long)KERNEL_HEAP_BASE;
    heap_limit = heap_begin + KERNEL_HEAP_SIZE;
    heap_free_list = 0;

    if (!heap_map_region())
        return;

    initial = heap_block_at(heap_begin);
    initial->size = KERNEL_HEAP_SIZE;
    initial->free = 1;
    initial->next_free = 0;
    heap_free_list = initial;
}

void *heap_alloc(unsigned long size)
{
    struct heap_block *block;
    struct heap_block *prev;
    struct heap_block *remainder;
    unsigned long need;

    if (size == 0 || heap_free_list == 0)
        return 0;

    need = heap_align_up(size + HEAP_BLOCK_HEADER_SIZE);
    if (need < HEAP_MIN_BLOCK_SIZE)
        need = HEAP_MIN_BLOCK_SIZE;

    for (prev = 0, block = heap_free_list; block != 0;
         prev = block, block = block->next_free)
    {
        if (block->size < need)
            continue;

        if (prev != 0)
            prev->next_free = block->next_free;
        else
            heap_free_list = block->next_free;

        /*
         * Split when the trailing free piece is large enough for header +
         * payload; otherwise allocate the whole free block (internal slack).
         */
        if (block->size - need >= HEAP_MIN_BLOCK_SIZE)
        {
            remainder = heap_block_at((unsigned char *)block + need);
            remainder->size = block->size - need;
            remainder->free = 1;
            remainder->next_free = heap_free_list;
            heap_free_list = remainder;
            block->size = need;
        }

        block->free = 0;
        block->next_free = 0;
        return (unsigned char *)block + HEAP_BLOCK_HEADER_SIZE;
    }

    return 0;
}

void heap_free(void *ptr)
{
    struct heap_block *block;

    if (ptr == 0)
        return;

    block = heap_block_at((unsigned char *)ptr - HEAP_BLOCK_HEADER_SIZE);

    if ((unsigned char *)block < heap_begin || (unsigned char *)block >= heap_limit)
        return;

    if (block->free)
        return;

    block->free = 1;
    block->next_free = heap_free_list;
    heap_free_list = block;

    heap_coalesce(block);
}
