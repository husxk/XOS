#include "mem/kmem_print.h"

#include "log/kprint.h"

void kheap_print_map_phys_failed(unsigned int page_index)
{
    kprint("kheap: phys page alloc failed at index %u\n", page_index);
}

void kheap_print_map_page_failed(unsigned int virt, unsigned long phys)
{
    kprint("kheap: map failed virt 0x%x phys 0x%x\n", virt, (unsigned int)phys);
}
