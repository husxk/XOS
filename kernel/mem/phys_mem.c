#include "mem/phys_mem.h"

#include "mem/kmem_utils.h"
#include "mem/phys_mem_err.h"
#include "mem/phys_map.h"

/*
 * 1ULL << 32 == 2^32 bytes == 4 GiB: exclusive upper bound on physical addresses
 * we track. This kernel is 32-bit i686 (not x86_64), so frame indices and
 * bitmap math use 32-bit types — not the full 64-bit values Multiboot mmap can
 * report. Clamp regions to [0, PHYS_MEM_ADDR_CAP). This is address-space width,
 * not a cap on how much RAM the machine has.
 */
#define PHYS_MEM_ADDR_CAP (1ULL << 32)

static unsigned char *phys_mem_bitmap;
static unsigned long phys_mem_bitmap_storage_base;
static unsigned long phys_mem_bitmap_storage_page_span;
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

/* One bit per frame; eight frame bits per bitmap byte (frame N → byte N/8). */
static unsigned char phys_mem_bitmap_frame_span_mask(unsigned char lo_bit, unsigned char hi_bit)
{
    return (unsigned char)(((unsigned char)0xFFu >> (7u - hi_bit)) &
                           ((unsigned char)0xFFu << lo_bit));
}

static void phys_mem_bitmap_apply_mask(unsigned long bitmap_byte, unsigned char mask, int used)
{
    if (used)
        phys_mem_bitmap[bitmap_byte] |= mask;
    else
        phys_mem_bitmap[bitmap_byte] &= (unsigned char)~mask;
}

static void phys_mem_bitmap_mark_frames(unsigned long start_frame, unsigned long end_frame, int used)
{
    unsigned long bitmap_byte_lo;
    unsigned long bitmap_byte_hi;
    unsigned long bitmap_byte;
    unsigned char middle_fill = used ? (unsigned char)0xFFu : 0u;

    if (phys_mem_bitmap == 0 || start_frame > end_frame)
        return;

    if (end_frame > phys_mem_max_frame)
        end_frame = phys_mem_max_frame;

    bitmap_byte_lo = start_frame / 8u;
    bitmap_byte_hi = end_frame / 8u;

    if (bitmap_byte_lo == bitmap_byte_hi)
    {
        unsigned char lo_bit = (unsigned char)(start_frame % 8u);
        unsigned char hi_bit = (unsigned char)(end_frame % 8u);
        unsigned char mask = phys_mem_bitmap_frame_span_mask(lo_bit, hi_bit);

        phys_mem_bitmap_apply_mask(bitmap_byte_lo, mask, used);
        return;
    }

    {
        unsigned char lo_bit = (unsigned char)(start_frame % 8u);
        unsigned char mask = (unsigned char)(0xFFu << lo_bit);

        phys_mem_bitmap_apply_mask(bitmap_byte_lo, mask, used);
    }

    for (bitmap_byte = bitmap_byte_lo + 1u; bitmap_byte < bitmap_byte_hi; bitmap_byte++)
        phys_mem_bitmap[bitmap_byte] = middle_fill;

    {
        unsigned char hi_bit = (unsigned char)(end_frame % 8u);
        unsigned char mask = (unsigned char)((1u << (hi_bit + 1u)) - 1u);

        phys_mem_bitmap_apply_mask(bitmap_byte_hi, mask, used);
    }
}

static void phys_mem_mark_range(unsigned long base, unsigned long length, int used)
{
    unsigned long end_phys;
    unsigned long start_frame;
    unsigned long end_frame;

    if (length == 0)
        return;

    end_phys = base + length;
    if (end_phys <= base)
        return;

    start_frame = phys_mem_align_down(base) / PHYS_MEM_PAGE_SIZE;
    end_phys = phys_mem_align_up(end_phys);
    if (end_phys == 0)
        return;

    end_frame = end_phys / PHYS_MEM_PAGE_SIZE;
    if (end_frame == 0)
        return;

    end_frame--;

    if (start_frame > phys_mem_max_frame)
        return;

    phys_mem_bitmap_mark_frames(start_frame, end_frame, used);
}

static unsigned long phys_mem_recount_free_frames(void)
{
    unsigned long free = 0;
    unsigned long byte;

    if (phys_mem_bitmap == 0)
        return 0;

    for (byte = 0; byte < phys_mem_bitmap_size_bytes; byte++)
    {
        unsigned char v = phys_mem_bitmap[byte];
        unsigned int bits = 8u;

        if (byte == phys_mem_bitmap_size_bytes - 1u)
        {
            unsigned long frame_count = phys_mem_max_frame + 1u;
            unsigned long tail = frame_count % 8u;

            bits = (tail == 0u) ? 8u : (unsigned int)tail;
        }

        for (unsigned int bit = 0; bit < bits; bit++)
        {
            if ((v & (unsigned char)(1u << bit)) == 0)
                free++;
        }
    }

    return free;
}

static void phys_mem_mark_range_used(unsigned long base, unsigned long length)
{
    phys_mem_mark_range(base, length, 1);
}

static void phys_mem_mark_range_free(unsigned long base, unsigned long length)
{
    phys_mem_mark_range(base, length, 0);
}

static void phys_mem_apply_reserves(const struct phys_mem_init_params *params)
{
    if (params->reserved == 0)
        return;

    for (unsigned int i = 0; i < params->reserved_count; i++)
    {
        const struct phys_mem_reserved_range *range = &params->reserved[i];

        if (range->length == 0)
            continue;

        phys_mem_mark_range_used(range->base, range->length);
    }
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

    phys_mem_max_frame = 0;
    phys_mem_frame_total = 0;

    for (unsigned int i = 0; i < region_count; i++)
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
static int phys_mem_boot_alloc_bitmap(unsigned long min_start,
                                      unsigned long *bitmap_storage_base)
{
    unsigned long bitmap_byte_length =
        phys_mem_bitmap_byte_length(phys_mem_frame_total);

    unsigned long bitmap_page_span =
        phys_mem_bitmap_page_span(bitmap_byte_length);

    unsigned int region_count = phys_map_region_count();

    phys_mem_bitmap_size_bytes = bitmap_byte_length;

    for (unsigned int i = 0; i < region_count; i++)
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

    for (unsigned int i = 0; i < region_count; i++)
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
    phys_mem_bitmap_storage_base = 0;
    phys_mem_bitmap_storage_page_span = 0;
    phys_mem_bitmap_size_bytes = 0;
    phys_mem_max_frame = 0;
    phys_mem_frame_total = 0;
    phys_mem_free_count = 0;
    phys_mem_alloc_hint = 0;
}

int phys_mem_init_from_params(const struct phys_mem_init_params *params)
{
    unsigned long bitmap_storage_base;
    unsigned long bitmap_page_span;

    if (params == 0)
        return PHYS_MEM_EINVAL;

    phys_mem_reset_state();
    phys_mem_scan_max_frame();

    if (phys_mem_frame_total == 0)
        return PHYS_MEM_ENOMAP;

    phys_mem_bitmap_size_bytes = phys_mem_bitmap_byte_length(phys_mem_frame_total);
    bitmap_page_span = phys_mem_bitmap_page_span(phys_mem_bitmap_size_bytes);

    if (!phys_mem_boot_alloc_bitmap(params->bitmap_min_phys, &bitmap_storage_base))
    {
        phys_mem_reset_state();
        return PHYS_MEM_ENOSPC;
    }

    phys_mem_bitmap = (unsigned char *)(unsigned long)bitmap_storage_base;

    phys_mem_bitmap_storage_base = bitmap_storage_base;
    phys_mem_bitmap_storage_page_span = bitmap_page_span;
    kmemset(phys_mem_bitmap, 0xFF, phys_mem_bitmap_size_bytes);

    phys_mem_mark_available_from_map();

    /* Reserve every page the bitmap occupies (page_span), not just byte_length. */
    phys_mem_mark_range_used(bitmap_storage_base, bitmap_page_span);
    phys_mem_apply_reserves(params);

    phys_mem_free_count = phys_mem_recount_free_frames();
    return PHYS_MEM_OK;
}

void phys_mem_stats(unsigned long *frame_total, unsigned long *free_count,
                    unsigned long *bitmap_size_bytes)
{
    if (frame_total != 0)
        *frame_total = phys_mem_frame_total;

    if (free_count != 0)
        *free_count = phys_mem_free_count;

    if (bitmap_size_bytes != 0)
        *bitmap_size_bytes = phys_mem_bitmap_size_bytes;
}

void phys_mem_bitmap_storage_span(unsigned long *base, unsigned long *page_span)
{
    if (base == 0 || page_span == 0)
        return;

    if (phys_mem_bitmap == 0)
    {
        *base = 0;
        *page_span = 0;
        return;
    }

    *base = phys_mem_bitmap_storage_base;
    *page_span = phys_mem_bitmap_storage_page_span;
}

static int phys_mem_run_is_free(unsigned long start_frame, unsigned long page_count)
{
    for (unsigned long i = 0; i < page_count; i++)
    {
        if (phys_mem_frame_is_used(start_frame + i))
            return 0;
    }

    return 1;
}

static int phys_mem_run_is_used(unsigned long start_frame, unsigned long page_count)
{
    for (unsigned long i = 0; i < page_count; i++)
    {
        if (!phys_mem_frame_is_used(start_frame + i))
            return 0;
    }

    return 1;
}

static void phys_mem_set_alloc_hint_after(unsigned long last_frame)
{
    phys_mem_alloc_hint = last_frame + 1u;
    if (phys_mem_alloc_hint > phys_mem_max_frame)
        phys_mem_alloc_hint = 0;
}

static void *phys_mem_take_frames(unsigned long start_frame, unsigned long page_count)
{
    for (unsigned long i = 0; i < page_count; i++)
        phys_mem_set_frame_used(start_frame + i, 1);

    phys_mem_set_alloc_hint_after(start_frame + page_count - 1u);

    return (void *)(unsigned long)phys_mem_frame_to_phys(start_frame);
}

static int phys_mem_run_fits(unsigned long start_frame, unsigned long page_count)
{
    unsigned long last_frame;

    if (page_count == 0)
        return 0;

    last_frame = start_frame + page_count - 1u;
    if (last_frame < start_frame)
        return 0;

    return last_frame <= phys_mem_max_frame;
}

void *phys_mem_alloc_pages(unsigned long page_count)
{
    unsigned long frame;
    unsigned long start;

    if (page_count == 0 || phys_mem_frame_total == 0 || phys_mem_bitmap == 0)
        return 0;

    start = phys_mem_alloc_hint;

    for (frame = start; frame <= phys_mem_max_frame; frame++)
    {
        if (!phys_mem_run_fits(frame, page_count))
            continue;

        if (phys_mem_run_is_free(frame, page_count))
            return phys_mem_take_frames(frame, page_count);
    }

    for (frame = 0; frame < start; frame++)
    {
        if (!phys_mem_run_fits(frame, page_count))
            continue;

        if (phys_mem_run_is_free(frame, page_count))
            return phys_mem_take_frames(frame, page_count);
    }

    return 0;
}

void *phys_mem_alloc_page(void)
{
    return phys_mem_alloc_pages(1);
}

int phys_mem_free_pages(void *base, unsigned long page_count)
{
    unsigned long phys = (unsigned long)base;
    unsigned long frame;

    if (page_count == 0)
        return PHYS_MEM_EINVAL;

    if ((phys & (PHYS_MEM_PAGE_SIZE - 1u)) != 0u)
        return PHYS_MEM_EINVAL;

    if (!phys_mem_phys_to_frame(phys, &frame))
        return PHYS_MEM_ERANGE;

    if (!phys_mem_run_fits(frame, page_count))
        return PHYS_MEM_ERANGE;

    if (!phys_mem_run_is_used(frame, page_count))
        return PHYS_MEM_ESTATE;

    for (unsigned long i = 0; i < page_count; i++)
        phys_mem_set_frame_used(frame + i, 0);

    return PHYS_MEM_OK;
}

int phys_mem_free_page(void *page)
{
    return phys_mem_free_pages(page, 1);
}
