#include "mem/phys_map.h"

#include "log/kprint.h"
#include "mem/multiboot2.h"

static struct phys_region regions[PHYS_MAP_REGION_MAX];
static unsigned int region_count;

static const struct multiboot_tag *next_tag(const struct multiboot_tag *tag,
                                            const unsigned char *end)
{
    unsigned int size;

    if ((const unsigned char *)tag >= end || tag->type == MULTIBOOT_TAG_TYPE_END)
        return 0;

    size = tag->size;
    if (size < sizeof(struct multiboot_tag))
        return 0;

    tag = (const struct multiboot_tag *)((const unsigned char *)tag + ((size + 7u) & ~7u));
    if ((const unsigned char *)tag >= end)
        return 0;

    return tag;
}

static void add_region(unsigned long long base, unsigned long long length, unsigned int type)
{
    if (region_count >= PHYS_MAP_REGION_MAX)
        return;

    regions[region_count].base = base;
    regions[region_count].length = length;
    regions[region_count].type = type;
    region_count++;
}

static void ingest_mmap_tag(const struct multiboot_tag_mmap *mmap,
                            const unsigned char *info_end)
{
    const unsigned char *entry;
    const unsigned char *entries_end;

    if (mmap->entry_size < sizeof(struct multiboot_mmap_entry))
        return;

    entry = (const unsigned char *)mmap + sizeof(struct multiboot_tag_mmap);
    entries_end = (const unsigned char *)mmap + mmap->size;

    while (entry + mmap->entry_size <= entries_end)
    {
        const struct multiboot_mmap_entry *e = (const struct multiboot_mmap_entry *)entry;

        add_region(e->base_addr, e->length, e->type);
        entry += mmap->entry_size;
    }
}

void phys_map_init(void)
{
    const struct multiboot_boot_info *info;
    const struct multiboot_tag *tag;
    const unsigned char *info_end;

    region_count = 0;

    if (multiboot2_info == 0)
        return;

    info = (const struct multiboot_boot_info *)(unsigned long)multiboot2_info;
    info_end = (const unsigned char *)info + info->total_size;
    tag = (const struct multiboot_tag *)(info + 1);

    while (tag != 0)
    {
        if (tag->type == MULTIBOOT_TAG_TYPE_MMAP)
            ingest_mmap_tag((const struct multiboot_tag_mmap *)tag, info_end);

        tag = next_tag(tag, info_end);
    }
}

unsigned int phys_map_region_count(void)
{
    return region_count;
}

const struct phys_region *phys_map_region(unsigned int index)
{
    if (index >= region_count)
        return 0;

    return &regions[index];
}

static void phys_map_print_size(unsigned long long bytes)
{
    kprint(" ");

    if (bytes >= 1024ULL * 1024ULL)
    {
        kprint_dec_u64(bytes / (1024ULL * 1024ULL));
        kprint(" MiB");
    }
    else if (bytes >= 1024ULL)
    {
        kprint_dec_u64(bytes / 1024ULL);
        kprint(" KiB");
    }
    else
    {
        kprint_dec_u64(bytes);
        kprint(" B");
    }
}

const char *phys_map_type_name(unsigned int type)
{
    switch (type)
    {
    case MULTIBOOT_MMAP_TYPE_AVAILABLE:
        return "AVAIL";

    case MULTIBOOT_MMAP_TYPE_RESERVED:
        return "RSVD";

    case MULTIBOOT_MMAP_TYPE_ACPI_RECLAIMABLE:
        return "ACPI";

    case MULTIBOOT_MMAP_TYPE_ACPI_NVS:
        return "NVS";

    case MULTIBOOT_MMAP_TYPE_BAD:
        return "BAD";

    default:
        return "UNK";
    }
}

unsigned long long phys_map_total_available(void)
{
    unsigned long long total = 0;
    unsigned int i;

    for (i = 0; i < region_count; i++)
    {
        if (regions[i].type == MULTIBOOT_MMAP_TYPE_AVAILABLE)
            total += regions[i].length;
    }

    return total;
}

void phys_map_print(void)
{
    unsigned int i;

    if (region_count == 0)
    {
        kprint("phys map: (none — multiboot2_info missing or no mmap tag)\n");
        return;
    }

    kprint("phys map regions:\n");

    for (i = 0; i < region_count; i++)
    {
        kprint("  ");
        kprint_hex64(regions[i].base);
        kprint(" + ");
        kprint_hex64(regions[i].length);
        kprint(" type ");
        kprint_hex32(regions[i].type);
        kprint(" ");
        kprint(phys_map_type_name(regions[i].type));
        phys_map_print_size(regions[i].length);
        kprint("\n");
    }

    unsigned long long total = phys_map_total_available();
    unsigned long long mib = total / (1024ULL * 1024ULL);

    kprint("available RAM: ");
    kprint_hex64(total);
    kprint(" bytes (");
    kprint_dec_u64(mib);
    kprint(" MiB)\n");
}
