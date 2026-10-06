#pragma once

void kheap_init(void);

void *kmalloc(unsigned long size);

void kfree(void *ptr);
