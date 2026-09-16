#pragma once

void kprint_init(void);

void kprint(const char *str);

void kprint_hex32(unsigned int value);

void kprint_hexdump(const void *addr, unsigned long size);
