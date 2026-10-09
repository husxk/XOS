#include "mem/phys_mem_kernel.h"

#include "mem/phys_mem.h"
#include "mem/phys_mem_err.h"
#include "mem/phys_mem_print.h"

void phys_mem_kernel_init(void)
{
    int err;

    err = phys_mem_init();
    if (err != PHYS_MEM_OK)
        phys_mem_print_error(err);
}
