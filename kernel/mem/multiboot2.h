#pragma once

/* Multiboot2 boot information (subset). See Multiboot2 spec. */

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289u

#define MULTIBOOT_TAG_TYPE_END   0u
#define MULTIBOOT_TAG_TYPE_MMAP  6u

#define MULTIBOOT_MMAP_TYPE_AVAILABLE          1u
#define MULTIBOOT_MMAP_TYPE_RESERVED           2u
#define MULTIBOOT_MMAP_TYPE_ACPI_RECLAIMABLE   3u
#define MULTIBOOT_MMAP_TYPE_ACPI_NVS           4u
#define MULTIBOOT_MMAP_TYPE_BAD                5u

struct multiboot_tag
{
    unsigned int type;
    unsigned int size;
};

struct multiboot_tag_mmap
{
    unsigned int type;
    unsigned int size;
    unsigned int entry_size;
    unsigned int entry_version;
};

struct multiboot_mmap_entry
{
    unsigned long long base_addr;
    unsigned long long length;
    unsigned int type;
    unsigned int reserved;
};

struct multiboot_boot_info
{
    unsigned int total_size;
    unsigned int reserved;
};

extern unsigned int multiboot2_info;
