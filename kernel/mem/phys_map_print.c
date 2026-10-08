#include "mem/phys_map.h"

#include "log/kprint.h"

static void phys_map_print_size(unsigned long long bytes)
{
    if (bytes >= 1024ULL * 1024ULL)
    {
        kprint(" %llu MiB", bytes / (1024ULL * 1024ULL));
    }
    else if (bytes >= 1024ULL)
    {
        kprint(" %llu KiB", bytes / 1024ULL);
    }
    else
    {
        kprint(" %llu B", bytes);
    }
}

void phys_map_print(void)
{
    unsigned int region_count = phys_map_region_count();
    unsigned int i;

    if (region_count == 0)
    {
        kprint("phys map: (none - multiboot2_info missing or no mmap tag)\n");
        return;
    }

    kprint("phys map regions:\n");

    for (i = 0; i < region_count; i++)
    {
        const struct phys_region *region = phys_map_region(i);

        if (region == 0)
            continue;

        kprint("  0x%016llx + 0x%016llx type %u %s",
               region->base, region->length,
               region->type, phys_map_type_name(region->type));
        phys_map_print_size(region->length);
        kputs("\n");
    }

    {
        unsigned long long total = phys_map_total_available();
        unsigned long long mib = total / (1024ULL * 1024ULL);

        kprint("available RAM: 0x%016llx bytes (%llu MiB)\n", total, mib);
    }
}
