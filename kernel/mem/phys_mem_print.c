#include "mem/phys_mem_print.h"

#include "log/kprint.h"
#include "mem/phys_mem.h"
#include "mem/phys_mem_err.h"

void phys_mem_print_stats(void)
{
    unsigned long frame_total;
    unsigned long free_count;
    unsigned long used;
    unsigned long bitmap_size_bytes;

    phys_mem_stats(&frame_total, &free_count, &bitmap_size_bytes);

    if (frame_total >= free_count)
        used = frame_total - free_count;
    else
        used = 0;

    kprint("phys_mem: frames %lu free %lu used %lu bitmap_size_bytes 0x%x\n",
           frame_total,
           free_count,
           used,
           (unsigned int)bitmap_size_bytes);
}

void phys_mem_print_error(int err)
{
    switch (err)
    {
    case PHYS_MEM_EINVAL:
        kprint("phys_mem: error EINVAL (invalid argument)\n");
        break;

    case PHYS_MEM_ENOMAP:
        kprint("phys_mem: error ENOMAP (no usable physical memory map)\n");
        break;

    case PHYS_MEM_ENOSPC:
        kprint("phys_mem: error ENOSPC (no space for frame bitmap)\n");
        break;

    case PHYS_MEM_ERANGE:
        kprint("phys_mem: error ERANGE (address out of range)\n");
        break;

    case PHYS_MEM_ESTATE:
        kprint("phys_mem: error ESTATE (free on unallocated frames)\n");
        break;

    default:
        kprint("phys_mem: error %d\n", err);
        break;
    }
}
