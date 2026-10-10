#include "mem/kmem.h"

#include "mem/kmem_utils.h"

#define KHEAP_ALLOC_ALIGN 8u

/*
 * Arena layout: [kheap_begin, kheap_limit) is a contiguous chain of blocks;
 * advance with (uint8_t *)block + block->size. Free blocks also hang off
 * kheap_free_list via next_free (only valid when free != 0).
 */
struct kheap_block
{
    unsigned long size; /* entire block including this header */
    unsigned char free;
    unsigned char _pad[3];
    struct kheap_block *next_free;
};

#define KHEAP_BLOCK_HEADER_SIZE                                              \
    (((unsigned long)sizeof(struct kheap_block) + (KHEAP_ALLOC_ALIGN - 1u)) & \
     ~(unsigned long)(KHEAP_ALLOC_ALIGN - 1u))

#define KHEAP_MIN_BLOCK_SIZE (KHEAP_BLOCK_HEADER_SIZE + KHEAP_ALLOC_ALIGN)

static unsigned char *kheap_begin;
static unsigned char *kheap_limit;
static struct kheap_block *kheap_free_list;

static unsigned long kheap_align_up(unsigned long n)
{
    return (n + (KHEAP_ALLOC_ALIGN - 1u)) & ~(KHEAP_ALLOC_ALIGN - 1u);
}

static void kheap_unlink_free(struct kheap_block *block)
{
    struct kheap_block *prev;
    struct kheap_block *cur;

    for (prev = 0, cur = kheap_free_list; cur != 0;
         prev = cur, cur = cur->next_free)
    {
        if (cur != block)
            continue;

        if (prev != 0)
            prev->next_free = block->next_free;
        else
            kheap_free_list = block->next_free;

        return;
    }
}

static struct kheap_block *kheap_block_at(unsigned char *addr)
{
    return (struct kheap_block *)(void *)addr;
}

static struct kheap_block *kheap_next_physical(struct kheap_block *block)
{
    unsigned char *next;

    next = (unsigned char *)block + block->size;
    if (next >= kheap_limit)
        return 0;

    return kheap_block_at(next);
}

static struct kheap_block *kheap_prev_physical(struct kheap_block *block)
{
    struct kheap_block *walk;

    if ((unsigned char *)block <= kheap_begin)
        return 0;

    walk = kheap_block_at(kheap_begin);
    while ((unsigned char *)walk + walk->size < (unsigned char *)block)
        walk = kheap_next_physical(walk);

    if ((unsigned char *)walk + walk->size != (unsigned char *)block)
        return 0;

    return walk;
}

static void kheap_coalesce(struct kheap_block *block)
{
    struct kheap_block *next;
    struct kheap_block *prev;

    next = kheap_next_physical(block);
    if (next != 0 && next->free)
    {
        kheap_unlink_free(next);
        block->size += next->size;
    }

    prev = kheap_prev_physical(block);
    if (prev != 0 && prev->free)
    {
        kheap_unlink_free(block);
        prev->size += block->size;
    }
}

int kheap_init_from_params(const struct kheap_init_params *params)
{
    struct kheap_block *initial;
    unsigned char *arena;
    unsigned long size;

    if (params == 0 || params->arena == 0)
        return KHEAP_EINVAL;

    size = params->size;
    if (size < KHEAP_MIN_BLOCK_SIZE)
        return KHEAP_EINVAL;

    arena = (unsigned char *)params->arena;
    if (((unsigned long)arena & (KHEAP_ALLOC_ALIGN - 1u)) != 0u)
        return KHEAP_EINVAL;

    kheap_begin = arena;
    kheap_limit = arena + size;
    kheap_free_list = 0;

    initial = kheap_block_at(kheap_begin);
    initial->size = size;
    initial->free = 1;
    initial->next_free = 0;
    kheap_free_list = initial;

    return KHEAP_OK;
}

void *kmalloc(unsigned long size)
{
    struct kheap_block *block;
    struct kheap_block *prev;
    struct kheap_block *remainder;
    unsigned long need;

    if (size == 0 || kheap_free_list == 0)
        return 0;

    need = kheap_align_up(size + KHEAP_BLOCK_HEADER_SIZE);
    if (need < KHEAP_MIN_BLOCK_SIZE)
        need = KHEAP_MIN_BLOCK_SIZE;

    for (prev = 0, block = kheap_free_list; block != 0;
         prev = block, block = block->next_free)
    {
        if (block->size < need)
            continue;

        if (prev != 0)
            prev->next_free = block->next_free;
        else
            kheap_free_list = block->next_free;

        if (block->size - need >= KHEAP_MIN_BLOCK_SIZE)
        {
            remainder = kheap_block_at((unsigned char *)block + need);
            remainder->size = block->size - need;
            remainder->free = 1;
            remainder->next_free = kheap_free_list;
            kheap_free_list = remainder;
            block->size = need;
        }

        block->free = 0;
        block->next_free = 0;
        return (unsigned char *)block + KHEAP_BLOCK_HEADER_SIZE;
    }

    return 0;
}

void *kzalloc(unsigned long size)
{
    void *ptr;

    ptr = kmalloc(size);
    if (ptr == 0)
        return 0;

    kmemset(ptr, 0, size);
    return ptr;
}

void kfree(void *ptr)
{
    struct kheap_block *block;

    if (ptr == 0)
        return;

    block = kheap_block_at((unsigned char *)ptr - KHEAP_BLOCK_HEADER_SIZE);

    if ((unsigned char *)block < kheap_begin || (unsigned char *)block >= kheap_limit)
        return;

    if (block->free)
        return;

    block->free = 1;
    block->next_free = kheap_free_list;
    kheap_free_list = block;

    kheap_coalesce(block);
}
