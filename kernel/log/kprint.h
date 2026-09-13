#pragma once

void kprint_init(void);

void kprint(const char *str);

void kprint_hexdump(const void *addr, unsigned long size);
