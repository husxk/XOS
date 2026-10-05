#pragma once

void heap_init(void);

void *heap_alloc(unsigned long size);

void heap_free(void *ptr);
