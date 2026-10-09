#include "mem/phys_mem_kernel.h"

#include "mem/mem_layout.h"
#include "mem/multiboot2.h"
#include "mem/multiboot2_boot.h"
#include "mem/phys_mem.h"
#include "mem/phys_mem_err.h"
#include "mem/phys_mem_print.h"

extern char _end[];
extern char stack_bottom[];
extern char stack_top[];

static unsigned long phys_mem_kernel_align_up(unsigned long addr)
{
    return (addr + (XOS_PAGE_SIZE - 1u)) & ~(unsigned long)(XOS_PAGE_SIZE - 1u);
}

static unsigned long phys_mem_kernel_align_down(unsigned long long addr)
{
    return (unsigned long)(addr & ~(unsigned long long)(XOS_PAGE_SIZE - 1u));
}

static unsigned long phys_mem_kernel_align_up_u64(unsigned long long addr)
{
    return (unsigned long)((addr + (unsigned long long)XOS_PAGE_SIZE - 1u) &
                           ~(unsigned long long)(XOS_PAGE_SIZE - 1u));
}

static unsigned int phys_mem_kernel_push_reserve(struct phys_mem_reserved_range *out,
                                               unsigned int cap, unsigned int n,
                                               unsigned long base, unsigned long length)
{
    if (length == 0 || n >= cap)
        return n;

    out[n].base = base;
    out[n].length = length;
    return n + 1;
}

static unsigned int phys_mem_kernel_fill_reserves(struct phys_mem_reserved_range *out,
                                                  unsigned int cap)
{
    unsigned int n = 0;

    n = phys_mem_kernel_push_reserve(out, cap, n, 0, XOS_PAGE_SIZE);

    unsigned long image_end = (unsigned long)&_end;

    if (image_end > KERNEL_LOAD_PHYS)
    {
        n = phys_mem_kernel_push_reserve(out, cap, n, KERNEL_LOAD_PHYS,
                                         image_end - KERNEL_LOAD_PHYS);
    }

    unsigned long stack_lo = (unsigned long)&stack_bottom;
    unsigned long stack_hi = (unsigned long)&stack_top;

    if (stack_hi > stack_lo)
    {
        n = phys_mem_kernel_push_reserve(out, cap, n, stack_lo, stack_hi - stack_lo);
    }

    if (multiboot2_info != 0)
    {
        const struct multiboot_boot_info *info =
            (const struct multiboot_boot_info *)(unsigned long)multiboot2_info;

        unsigned long info_start =
            phys_mem_kernel_align_down((unsigned long long)multiboot2_info);

        unsigned long info_end = phys_mem_kernel_align_up_u64(
            (unsigned long long)multiboot2_info + (unsigned long long)info->total_size);

        if (info_end > info_start)
        {
            n = phys_mem_kernel_push_reserve(out, cap, n, info_start,
                                             info_end - info_start);
        }
    }

    return n;
}

void phys_mem_kernel_init(void)
{
    struct phys_mem_reserved_range reserved[PHYS_MEM_RESERVED_MAX];
    struct phys_mem_init_params params;
    int err;

    params.bitmap_min_phys = phys_mem_kernel_align_up((unsigned long)&_end);
    params.reserved = reserved;
    params.reserved_count = phys_mem_kernel_fill_reserves(reserved, PHYS_MEM_RESERVED_MAX);

    err = phys_mem_init_from_params(&params);

    if (err != PHYS_MEM_OK)
        phys_mem_print_error(err);
}
