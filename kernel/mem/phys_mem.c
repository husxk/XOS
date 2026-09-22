#include "mem/phys_mem.h"

#include "log/kprint.h"
#include "mem/kmem.h"
#include "mem/multiboot2.h"
#include "mem/phys_map.h"

/*
 * 1ULL << 32 == 2^32 bytes == 4 GiB: exclusive upper bound on physical addresses
 * we track. This kernel is 32-bit i686 (not x86_64), so frame indices and
 * bitmap math use 32-bit types — not the full 64-bit values Multiboot mmap can
 * report. Clamp regions to [0, PHYS_MEM_ADDR_CAP). This is address-space width,
 * not a cap on how much RAM the machine has.
 */
#define PHYS_MEM_ADDR_CAP (1ULL << 32)
#define KERNEL_LOAD_PHYS  0x1000u

extern char _end[];
extern char stack_bottom[];
extern char stack_top[];

static unsigned char *phys_mem_bitmap;
static unsigned long phys_mem_bitmap_size_bytes;
static unsigned long phys_mem_max_frame;
static unsigned long phys_mem_frame_total;
static unsigned long phys_mem_free_count;
static unsigned long phys_mem_alloc_hint;

static unsigned long long phys_mem_cap_end(unsigned long long end)
{
    if (end > PHYS_MEM_ADDR_CAP)
        return PHYS_MEM_ADDR_CAP;

    return end;
}

static unsigned long long phys_mem_align_down_u64(unsigned long long addr)
{
    return addr & ~(unsigned long long)(PHYS_MEM_PAGE_SIZE - 1u);
}

static unsigned long long phys_mem_align_up_u64(unsigned long long addr)
{
    return (addr + (unsigned long long)PHYS_MEM_PAGE_SIZE - 1u) &
           ~(unsigned long long)(PHYS_MEM_PAGE_SIZE - 1u);
}

static unsigned long phys_mem_align_down(unsigned long addr)
{
    return (unsigned long)phys_mem_align_down_u64(addr);
}

static unsigned long phys_mem_align_up(unsigned long addr)
{
    return (unsigned long)phys_mem_align_up_u64(addr);
}

static unsigned long phys_mem_frame_to_phys(unsigned long frame)
{
    return frame * PHYS_MEM_PAGE_SIZE;
}

static int phys_mem_phys_to_frame(unsigned long phys, unsigned long *frame_out)
{
    unsigned long frame;

    if ((phys & (PHYS_MEM_PAGE_SIZE - 1u)) != 0u)
        return 0;

    if (phys_mem_frame_total == 0)
        return 0;

    frame = phys / PHYS_MEM_PAGE_SIZE;
    if (frame > phys_mem_max_frame)
        return 0;

    *frame_out = frame;
    return 1;
}

static int phys_mem_frame_is_used(unsigned long frame)
{
    unsigned long byte = frame / 8u;
    unsigned char bit = (unsigned char)(frame % 8u);
    unsigned char mask = (unsigned char)(1u << bit);

    return (phys_mem_bitmap[byte] & mask) != 0;
}

static void phys_mem_set_frame_used(unsigned long frame, int used)
{
    unsigned long byte = frame / 8u;
    unsigned char bit = (unsigned char)(frame % 8u);
    unsigned char mask = (unsigned char)(1u << bit);

    int was_used = (phys_mem_bitmap[byte] & mask) != 0;

    if (used)
    {
        if (!was_used)
            phys_mem_free_count--;

        phys_mem_bitmap[byte] |= mask;
    }
    else
    {
        if (was_used)
            phys_mem_free_count++;

        phys_mem_bitmap[byte] &= (unsigned char)~mask;
    }
}

static void phys_mem_mark_range(unsigned long base, unsigned long length, int used)
{
    unsigned long end_phys;
    unsigned long frame;

    if (length == 0)
        return;

    end_phys = base + length;
    if (end_phys <= base)
        return;

    frame = phys_mem_align_down(base) / PHYS_MEM_PAGE_SIZE;

    while (frame <= phys_mem_max_frame)
    {
        unsigned long frame_phys = phys_mem_frame_to_phys(frame);

        if (frame_phys >= end_phys)
            break;

        phys_mem_set_frame_used(frame, used);
        frame++;
    }
}

static void phys_mem_mark_range_used(unsigned long base, unsigned long length)
{
    phys_mem_mark_range(base, length, 1);
}

static void phys_mem_mark_range_free(unsigned long base, unsigned long length)
{
    phys_mem_mark_range(base, length, 0);
}

static void phys_mem_reserve_kernel_image(void)
{
    unsigned long start = KERNEL_LOAD_PHYS;
    unsigned long end = (unsigned long)&_end;

    if (end <= start)
        return;

    phys_mem_mark_range_used(start, end - start);
}

static void phys_mem_reserve_kernel_stack(void)
{
    unsigned long start = (unsigned long)&stack_bottom;
    unsigned long end = (unsigned long)&stack_top;

    if (end <= start)
        return;

    phys_mem_mark_range_used(start, end - start);
}

static void phys_mem_reserve_multiboot_info(void)
{
    const struct multiboot_boot_info *info;
    unsigned long info_start;
    unsigned long info_end;

    if (multiboot2_info == 0)
        return;

    info = (const struct multiboot_boot_info *)(unsigned long)multiboot2_info;
    info_start = (unsigned long)phys_mem_align_down_u64((unsigned long long)multiboot2_info);
    info_end = (unsigned long)phys_mem_align_up_u64((unsigned long long)multiboot2_info +
                                                    (unsigned long long)info->total_size);

    if (info_end <= info_start)
        return;

    phys_mem_mark_range_used(info_start, info_end - info_start);
}

static void phys_mem_apply_fixed_reserves(void)
{
    /*
     * Never hand out physical page 0: (void *)0x0 is NULL, so alloc cannot
     * return it without a separate error channel.
     */
    phys_mem_mark_range_used(0, PHYS_MEM_PAGE_SIZE);

    phys_mem_reserve_kernel_image();
    phys_mem_reserve_kernel_stack();
    phys_mem_reserve_multiboot_info();
}

/*
 * Bitmap index is phys / PAGE_SIZE, so we need one bit for every frame index
 * from 0 up to the top of available RAM — not a per-region page count.
 * Holes (e.g. below 1 MiB) still consume bits; they stay "used" after init.
 */
static void phys_mem_scan_max_frame(void)
{
    unsigned int region_count = phys_map_region_count();
    unsigned long long max_end = 0;
    unsigned int i;

    phys_mem_max_frame = 0;
    phys_mem_frame_total = 0;

    for (i = 0; i < region_count; i++)
    {
        const struct phys_region *region = phys_map_region(i);
        unsigned long long end;

        if (region == 0)
            continue;

        if (region->type != MULTIBOOT_MMAP_TYPE_AVAILABLE)
            continue;

        end = region->base + region->length;
        if (end > max_end)
            max_end = end;
    }

    max_end = phys_mem_cap_end(max_end);
    if (max_end == 0)
        return;

    unsigned long long frame_count =
        phys_mem_align_up_u64(max_end) / (unsigned long long)PHYS_MEM_PAGE_SIZE;

    phys_mem_frame_total = (unsigned long)frame_count;
    phys_mem_max_frame = phys_mem_frame_total - 1u;
}

static unsigned long phys_mem_bitmap_byte_length(unsigned long frame_total)
{
    return (frame_total + 7u) / 8u;
}

/*
 * Returns how many bytes of physical memory the bitmap covers, as a byte count
 * (not a page count): align_up(bitmap_byte_length) to a 4 KiB boundary.
 * Example: 5000 B of bits -> 8192 (2 pages); 65 B -> 4096 (1 page).
 * alloc/free track whole pages, so placement and mark-used use this span so
 * every touched page lies fully in available RAM.
 */
static unsigned long phys_mem_bitmap_page_span(unsigned long bitmap_byte_length)
{
    return phys_mem_align_up(bitmap_byte_length);
}

/*
 * Pick a physical address for the bitmap before the PMM is operational
 */
static int phys_mem_boot_alloc_bitmap(unsigned long *bitmap_storage_base)
{
    unsigned long bitmap_byte_length =
        phys_mem_bitmap_byte_length(phys_mem_frame_total);

    unsigned long bitmap_page_span =
        phys_mem_bitmap_page_span(bitmap_byte_length);

    unsigned int region_count = phys_map_region_count();
    unsigned long min_start = phys_mem_align_up((unsigned long)&_end);
    unsigned int i;

    phys_mem_bitmap_size_bytes = bitmap_byte_length;

    for (i = 0; i < region_count; i++)
    {
        const struct phys_region *region = phys_map_region(i);
        unsigned long long region_end;
        unsigned long candidate;
        unsigned long bitmap_end;

        if (region == 0)
            continue;

        if (region->type != MULTIBOOT_MMAP_TYPE_AVAILABLE)
            continue;

        candidate = (unsigned long)phys_mem_align_up_u64(region->base);
        if (candidate < min_start)
            candidate = min_start;

        region_end = region->base + region->length;

        /* [candidate, bitmap_end) must lie entirely in this available region. */
        bitmap_end = candidate + bitmap_page_span;
        if ((unsigned long long)bitmap_end > region_end)
            continue;

        *bitmap_storage_base = candidate;
        return 1;
    }

    return 0;
}

static void phys_mem_mark_available_from_map(void)
{
    unsigned int region_count = phys_map_region_count();
    unsigned int i;

    for (i = 0; i < region_count; i++)
    {
        const struct phys_region *region = phys_map_region(i);
        unsigned long base;
        unsigned long length;

        if (region == 0)
            continue;

        if (region->type != MULTIBOOT_MMAP_TYPE_AVAILABLE)
            continue;

        if (region->length == 0)
            continue;

        if (region->base >= PHYS_MEM_ADDR_CAP)
            continue;

        base = (unsigned long)region->base;
        length = (unsigned long)region->length;

        if ((unsigned long long)base + (unsigned long long)length > PHYS_MEM_ADDR_CAP)
            length = (unsigned long)(PHYS_MEM_ADDR_CAP - (unsigned long long)base);

        phys_mem_mark_range_free(base, length);
    }
}

static void phys_mem_reset_state(void)
{
    phys_mem_bitmap = 0;
    phys_mem_bitmap_size_bytes = 0;
    phys_mem_max_frame = 0;
    phys_mem_frame_total = 0;
    phys_mem_free_count = 0;
    phys_mem_alloc_hint = 0;
}

void phys_mem_init(void)
{
    unsigned long bitmap_storage_base;
    unsigned long bitmap_page_span;

    phys_mem_scan_max_frame();

    if (phys_mem_frame_total == 0)
        return;

    if (!phys_mem_boot_alloc_bitmap(&bitmap_storage_base))
    {
        kprint("phys_mem: no space for bitmap\n");
        phys_mem_reset_state();
        return;
    }

    bitmap_page_span = phys_mem_bitmap_page_span(phys_mem_bitmap_size_bytes);
    phys_mem_bitmap = (unsigned char *)(unsigned long)bitmap_storage_base;
    kmemset(phys_mem_bitmap, 0xFF, phys_mem_bitmap_size_bytes);

    phys_mem_mark_available_from_map();

    /* Reserve every page the bitmap occupies (page_span), not just byte_length. */
    phys_mem_mark_range_used(bitmap_storage_base, bitmap_page_span);
    phys_mem_apply_fixed_reserves();
}

static void *phys_mem_take_frame(unsigned long frame)
{
    phys_mem_set_frame_used(frame, 1);
    phys_mem_alloc_hint = frame + 1u;
    if (phys_mem_alloc_hint > phys_mem_max_frame)
        phys_mem_alloc_hint = 0;

    return (void *)(unsigned long)phys_mem_frame_to_phys(frame);
}

void *phys_mem_alloc_page(void)
{
    unsigned long frame;
    unsigned long start;

    if (phys_mem_frame_total == 0 || phys_mem_bitmap == 0)
        return 0;

    start = phys_mem_alloc_hint;

    for (frame = start; frame <= phys_mem_max_frame; frame++)
    {
        if (!phys_mem_frame_is_used(frame))
            return phys_mem_take_frame(frame);
    }

    for (frame = 0; frame < start; frame++)
    {
        if (!phys_mem_frame_is_used(frame))
            return phys_mem_take_frame(frame);
    }

    return 0;
}

void phys_mem_free_page(void *page)
{
    unsigned long phys = (unsigned long)page;
    unsigned long frame;

    if ((phys & (PHYS_MEM_PAGE_SIZE - 1u)) != 0u)
    {
        kprint("phys_mem: free ignored (unaligned)\n");
        return;
    }

    if (!phys_mem_phys_to_frame(phys, &frame))
    {
        kprint("phys_mem: free ignored (out of range)\n");
        return;
    }

    if (!phys_mem_frame_is_used(frame))
    {
        kprint("phys_mem: free ignored (not allocated)\n");
        return;
    }

    phys_mem_set_frame_used(frame, 0);
}

void phys_mem_print_stats(void)
{
    unsigned long used;

    if (phys_mem_frame_total >= phys_mem_free_count)
        used = phys_mem_frame_total - phys_mem_free_count;
    else
        used = 0;

    kprint("phys_mem: frames %lu free %lu used %lu bitmap_size_bytes 0x%x\n",
           phys_mem_frame_total,
           phys_mem_free_count,
           used,
           (unsigned int)phys_mem_bitmap_size_bytes);
}
