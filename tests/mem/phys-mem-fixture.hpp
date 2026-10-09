#pragma once

#include "mem/multiboot2.h"
#include "mem/phys_map.h"
#include "mem/phys_mem.h"
#include "mem/phys_mem_err.h"

#include "multiboot-mmap-fixture.hpp"

#include <cstdint>

namespace xos_test {

/*
 * Host PMM tests: AVAIL mmap covers [ram, ram + ram_size). Identity map — phys
 * addresses in the map must be the real host addresses of ram (not QEMU layout).
 * On 64-bit hosts, ram must lie below 4 GiB to match PHYS_MEM_ADDR_CAP.
 */
inline int phys_mem_test_init_from_ram(unsigned char *ram, unsigned long ram_size,
                                       unsigned char *multiboot_bytes,
                                       unsigned int multiboot_cap,
                                       const struct phys_mem_reserved_range *reserved,
                                       unsigned int reserved_count)
{
    const multiboot_mmap_entry entries[] = {
        {(unsigned long long)(uintptr_t)ram, (unsigned long long)ram_size,
         MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
    };

    const multiboot_boot_info *info;
    struct phys_mem_init_params params;

    info = multiboot_build_mmap_info(multiboot_bytes, multiboot_cap, entries, 1u, 0);
    if (info == 0)
        return PHYS_MEM_EINVAL;

    phys_map_init_from_info(info);

    params.bitmap_min_phys = (unsigned long)(uintptr_t)ram;
    params.reserved = reserved;
    params.reserved_count = reserved_count;

    return phys_mem_init_from_params(&params);
}

} /* namespace xos_test */
