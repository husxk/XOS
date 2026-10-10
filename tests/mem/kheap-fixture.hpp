#pragma once

#include "mem/kmem.h"

namespace xos_test {

inline int kheap_test_init_from_arena(unsigned char *arena, unsigned long size)
{
    struct kheap_init_params params;

    params.arena = arena;
    params.size = size;
    return kheap_init_from_params(&params);
}

} /* namespace xos_test */
