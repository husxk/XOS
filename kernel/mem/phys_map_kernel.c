#include "mem/phys_map.h"
#include "mem/multiboot2_boot.h"

void phys_map_kernel_init(void)
{
    if (multiboot2_info == 0)
    {
        phys_map_init_from_info(0);
        return;
    }

    phys_map_init_from_info(
        (const struct multiboot_boot_info *)(unsigned long)multiboot2_info);
}
