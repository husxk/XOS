#include "mem/kmem_kernel.h"

#include "mem/kmem.h"
#include "mem/kmem_print.h"
#include "mem/mem_layout.h"
#include "mem/paging.h"
#include "mem/phys_mem.h"

void kheap_init(void)
{
    struct kheap_init_params params;

    if (!kheap_map_region())
        return;

    params.arena = (void *)(unsigned long)KERNEL_HEAP_BASE;
    params.size = KERNEL_HEAP_SIZE;
    kheap_init_from_params(&params);
}

int kheap_map_region(void)
{
    unsigned long i;

    for (i = 0; i < KERNEL_HEAP_PAGE_COUNT; i++)
    {
        unsigned long virt = KERNEL_HEAP_BASE + i * XOS_PAGE_SIZE;
        void *phys = phys_mem_alloc_page();

        if (phys == 0)
        {
            kheap_print_map_phys_failed((unsigned int)i);
            return 0;
        }

        if (!paging_map_page(virt, (unsigned long)phys))
        {
            kheap_print_map_page_failed((unsigned int)virt, (unsigned long)phys);
            phys_mem_free_pages(phys, 1);
            return 0;
        }
    }

    return 1;
}
