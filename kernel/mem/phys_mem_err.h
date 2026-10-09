#pragma once

/* phys_mem_* return codes (0 = success). */

#define PHYS_MEM_OK 0

#define PHYS_MEM_EINVAL 1   /* invalid argument (e.g. zero page count, unaligned) */
#define PHYS_MEM_ENOMAP 2   /* no frame span from phys_map */
#define PHYS_MEM_ENOSPC 3   /* no RAM region fits the frame bitmap */
#define PHYS_MEM_ERANGE 4   /* physical address outside tracked frames */
#define PHYS_MEM_ESTATE 5   /* free on frames that are not marked allocated */
