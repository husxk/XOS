#pragma once

#include "mem/multiboot2.h"

#include <cstring>

namespace xos_test {

/* Multiboot2 tag sizes and tag boundaries are multiples of 8 (see spec). */
inline unsigned int multiboot_align_up8(unsigned int n)
{
    return (n + 7u) & ~7u;
}

/*
 * Lay out a Multiboot2 boot info header, one MMAP tag (with the given entries),
 * and an END tag in bytes[0..). Returns info or null if bytes_cap is too small.
 */
inline const multiboot_boot_info *multiboot_build_mmap_info(
    unsigned char *bytes, unsigned int bytes_cap,
    const multiboot_mmap_entry *entries, unsigned int entry_count,
    unsigned int *out_size)
{
    if (bytes == 0 || bytes_cap < sizeof(multiboot_boot_info) + 8u || entry_count == 0 ||
        entries == 0)
        return 0;

    /* Tag size field includes header + all entries; pad so the next tag is 8-byte aligned. */
    const unsigned int entry_size = (unsigned int)sizeof(multiboot_mmap_entry);
    const unsigned int mmap_size =
        multiboot_align_up8((unsigned int)sizeof(multiboot_tag_mmap) +
                            entry_size * entry_count);

    /* boot_info | mmap tag | END tag (8 bytes). */
    const unsigned int total =
        (unsigned int)sizeof(multiboot_boot_info) + mmap_size + 8u;

    if (total > bytes_cap)
        return 0;

    std::memset(bytes, 0, total);

    unsigned char *cursor = bytes;
    multiboot_boot_info *info = reinterpret_cast<multiboot_boot_info *>(cursor);
    cursor += sizeof(multiboot_boot_info);

    /* phys_map_init_from_info() walks tags starting at (info + 1). */
    multiboot_tag_mmap *mmap = reinterpret_cast<multiboot_tag_mmap *>(cursor);
    mmap->type = MULTIBOOT_TAG_TYPE_MMAP;
    mmap->size = mmap_size;
    mmap->entry_size = entry_size;
    mmap->entry_version = 0;

    unsigned char *entry_cursor = cursor + sizeof(multiboot_tag_mmap);
    for (unsigned int i = 0; i < entry_count; i++)
    {
        std::memcpy(entry_cursor, &entries[i], sizeof(entries[i]));
        entry_cursor += entry_size;
    }

    cursor += mmap_size;

    /* END tag stops the walk in next_tag(); size must be at least 8. */
    multiboot_tag *end_tag = reinterpret_cast<multiboot_tag *>(cursor);
    end_tag->type = MULTIBOOT_TAG_TYPE_END;
    end_tag->size = 8u;

    /* Parser uses info + total_size as the exclusive end of the info block. */
    info->total_size = total;
    info->reserved = 0;

    if (out_size != 0)
        *out_size = total;

    return info;
}

} /* namespace xos_test */
