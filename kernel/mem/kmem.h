#pragma once

#include "mem/kheap_err.h"

struct kheap_init_params
{
    void *arena;
    unsigned long size;
};

#ifdef __cplusplus
extern "C" {
#endif

int kheap_init_from_params(const struct kheap_init_params *params);

void kheap_init(void);

void *kmalloc(unsigned long size);

void *kzalloc(unsigned long size);

void kfree(void *ptr);

#ifdef __cplusplus
}
#endif
