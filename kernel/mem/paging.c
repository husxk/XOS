#include "mem/paging.h"

#include "log/kprint.h"
#include "mem/kmem_utils.h"
#include "mem/mem_layout.h"
#include "mem/multiboot2.h"
#include "mem/multiboot2_boot.h"
#include "mem/phys_mem.h"

#define PAGING_OFFSET_BITS 12u
#define PAGING_PT_INDEX_BITS 10u
#define PAGING_PD_INDEX_BITS 10u

#define PAGING_OFFSET_MASK ((1u << PAGING_OFFSET_BITS) - 1u)
#define PAGING_PT_INDEX_MASK ((1u << PAGING_PT_INDEX_BITS) - 1u)
#define PAGING_PD_INDEX_MASK ((1u << PAGING_PD_INDEX_BITS) - 1u)

#define PAGING_PT_INDEX_SHIFT PAGING_OFFSET_BITS
#define PAGING_PD_INDEX_SHIFT (PAGING_OFFSET_BITS + PAGING_PT_INDEX_BITS)

#define PAGING_PRESENT (1u << 0)
#define PAGING_WRITABLE (1u << 1)
#define PAGING_USER (1u << 2)
#define PAGING_ADDR_MASK 0xfffff000u

/* USER clear: supervisor-only (CPL 0-2). */
#define PAGING_KERNEL_FLAGS (PAGING_PRESENT | PAGING_WRITABLE)
#define PAGING_TABLE_FLAGS (PAGING_PRESENT | PAGING_WRITABLE)

#define VGA_TEXT_BUFFER_PHYS 0x000b8000u

typedef unsigned int paging_entry_t;

extern char _end[];
extern char stack_bottom[];
extern char stack_top[];

static paging_entry_t *page_directory;
static unsigned long page_directory_phys;

/* Phys frame of the next all-zero L2 page; cleared via PAGING_PT_STAGING_VA. */
static unsigned long spare_phys;
static int staging_ready;

static unsigned long long paging_align_down_u64(unsigned long long addr)
{
    return addr & ~(unsigned long long)(XOS_PAGE_SIZE - 1u);
}

static unsigned long long paging_align_up_u64(unsigned long long addr)
{
    return (addr + (unsigned long long)XOS_PAGE_SIZE - 1u) &
           ~(unsigned long long)(XOS_PAGE_SIZE - 1u);
}

static unsigned int paging_pd_index(unsigned long virt)
{
    return (unsigned int)((virt >> PAGING_PD_INDEX_SHIFT) & PAGING_PD_INDEX_MASK);
}

static unsigned int paging_pt_index(unsigned long virt)
{
    return (unsigned int)((virt >> PAGING_PT_INDEX_SHIFT) & PAGING_PT_INDEX_MASK);
}

static paging_entry_t paging_make_entry(unsigned long phys, unsigned int flags)
{
    return (paging_entry_t)(phys & PAGING_ADDR_MASK) | (flags & ~PAGING_ADDR_MASK);
}

static unsigned long paging_entry_frame(paging_entry_t entry)
{
    return (unsigned long)(entry & PAGING_ADDR_MASK);
}

static int paging_entry_present(paging_entry_t entry)
{
    return (entry & PAGING_PRESENT) != 0;
}

static int paging_map_identity(unsigned long phys);
static int paging_map_page_internal(unsigned long virt, unsigned long phys);

static int paging_cr0_pg(void)
{
    unsigned long cr0;

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    return (cr0 & 0x80000000u) != 0;
}

/* Map phys at the fixed staging VA and zero the page (safe while CR0.PG is set). */
static int paging_pt_staging_map(unsigned long phys)
{
    if (!paging_map_page(PAGING_PT_STAGING_VA, phys))
        return 0;

    kmemset((void *)(unsigned long)PAGING_PT_STAGING_VA, 0, XOS_PAGE_SIZE);
    return 1;
}

static int paging_pt_spare_refill(void)
{
    void *frame;

    frame = phys_mem_alloc_page();
    if (frame == 0)
        return 0;

    if (!paging_pt_staging_map((unsigned long)frame))
        return 0;

    spare_phys = (unsigned long)frame;
    return 1;
}

static void paging_pt_staging_init(void)
{
    void *frame;

    spare_phys = 0;
    staging_ready = 0;

    frame = phys_mem_alloc_page();
    if (frame == 0)
    {
        kprint("paging: PT staging alloc failed\n");
        return;
    }

    if (!paging_pt_staging_map((unsigned long)frame))
    {
        kprint("paging: PT staging map failed\n");
        return;
    }

    spare_phys = (unsigned long)frame;
    staging_ready = 1;
}

static paging_entry_t *paging_get_or_create_pt(unsigned int pd_index)
{
    paging_entry_t pde;
    paging_entry_t *pt;
    void *pt_frame;
    unsigned long pt_phys;

    pde = page_directory[pd_index];
    if (paging_entry_present(pde))
        return (paging_entry_t *)paging_entry_frame(pde);

    /*
     * With paging on, arbitrary phys cannot be cleared directly. spare_phys
     * holds a frame already zeroed through PAGING_PT_STAGING_VA. Refill the
     * staging mapping before installing the PDE so the staging slot never
     * aliases a live page table. Identity-map the new L2 so later pt[] writes
     * through (paging_entry_t *)pt_phys remain valid.
     */
    if (paging_cr0_pg())
    {
        if (!staging_ready || spare_phys == 0)
        {
            kprint("paging: PT spare unavailable\n");
            return 0;
        }

        pt_phys = spare_phys;
        if (!paging_pt_spare_refill())
        {
            kprint("paging: PT spare refill failed\n");
            return 0;
        }

        page_directory[pd_index] = paging_make_entry(pt_phys, PAGING_TABLE_FLAGS);
        if (!paging_map_identity(pt_phys))
            return 0;

        return (paging_entry_t *)pt_phys;
    }

    /*
     * During paging_init (CR0.PG clear): identity-mapped kernel uses linear
     * phys addresses, so allocate a frame and kmemset it in place.
     */
    pt_frame = phys_mem_alloc_page();
    if (pt_frame == 0)
        return 0;

    pt_phys = (unsigned long)pt_frame;
    pt = (paging_entry_t *)pt_phys;
    kmemset(pt, 0, XOS_PAGE_SIZE);

    page_directory[pd_index] = paging_make_entry(pt_phys, PAGING_TABLE_FLAGS);

    if (!paging_map_identity(pt_phys))
        return 0;

    return pt;
}

static int paging_map_page_internal(unsigned long virt, unsigned long phys)
{
    paging_entry_t *pt;
    unsigned int pd_i;
    unsigned int pt_i;

    if (page_directory_phys == 0)
        return 0;

    if ((virt & (XOS_PAGE_SIZE - 1u)) != 0u)
        return 0;
    if ((phys & (XOS_PAGE_SIZE - 1u)) != 0u)
        return 0;

    pd_i = paging_pd_index(virt);
    pt_i = paging_pt_index(virt);

    pt = paging_get_or_create_pt(pd_i);
    if (pt == 0)
        return 0;

    pt[pt_i] = paging_make_entry(phys, PAGING_KERNEL_FLAGS);
    return 1;
}

int paging_map_page(unsigned long virt, unsigned long phys)
{
    if (!paging_map_page_internal(virt, phys))
        return 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
    return 1;
}

static int paging_map_identity(unsigned long phys)
{
    return paging_map_page_internal(phys, phys);
}

static void paging_map_identity_range(unsigned long base, unsigned long length)
{
    unsigned long end;
    unsigned long addr;

    if (length == 0)
        return;

    end = base + length;
    if (end <= base)
        return;

    addr = (unsigned long)paging_align_down_u64(base);
    for (; addr < end; addr += XOS_PAGE_SIZE)
    {
        if (!paging_map_identity(addr))
        {
            kprint("paging: map failed at 0x%x\n", (unsigned int)addr);
            return;
        }
    }
}

static void paging_map_multiboot_info(void)
{
    const struct multiboot_boot_info *info;
    unsigned long info_start;
    unsigned long info_end;

    if (multiboot2_info == 0)
        return;

    info = (const struct multiboot_boot_info *)(unsigned long)multiboot2_info;
    info_start = (unsigned long)paging_align_down_u64((unsigned long long)multiboot2_info);
    info_end = (unsigned long)paging_align_up_u64((unsigned long long)multiboot2_info +
                                                  (unsigned long long)info->total_size);

    if (info_end <= info_start)
        return;

    paging_map_identity_range(info_start, info_end - info_start);
}

/* Identity-map boot essentials only (not all Multiboot AVAILABLE RAM). */
static void paging_map_boot_regions(void)
{
    unsigned long start;
    unsigned long end;
    unsigned long bitmap_base;
    unsigned long bitmap_span;

    start = KERNEL_LOAD_PHYS;
    end = (unsigned long)&_end;
    if (end > start)
        paging_map_identity_range(start, end - start);

    start = (unsigned long)&stack_bottom;
    end = (unsigned long)&stack_top;
    if (end > start)
        paging_map_identity_range(start, end - start);

    paging_map_multiboot_info();

    phys_mem_bitmap_storage_span(&bitmap_base, &bitmap_span);
    if (bitmap_base != 0 && bitmap_span != 0)
        paging_map_identity_range(bitmap_base, bitmap_span);

    (void)paging_map_identity(VGA_TEXT_BUFFER_PHYS);
}

void paging_init(void)
{
    void *pd_frame;

    page_directory = 0;
    page_directory_phys = 0;

    pd_frame = phys_mem_alloc_page();
    if (pd_frame == 0)
    {
        kprint("paging: page directory alloc failed\n");
        return;
    }

    page_directory_phys = (unsigned long)pd_frame;
    page_directory = (paging_entry_t *)page_directory_phys;
    kmemset(page_directory, 0, XOS_PAGE_SIZE);

    paging_map_boot_regions();

    /* Page directory page must be reachable at VA == page_directory_phys too. */
    (void)paging_map_identity(page_directory_phys);

    paging_pt_staging_init();
}

void paging_enable(void)
{
    if (page_directory_phys == 0)
    {
        kprint("paging: enable skipped (no page directory)\n");
        return;
    }

    __asm__ volatile(
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0"
        :
        : "r"(page_directory_phys)
        : "eax", "memory");
}
