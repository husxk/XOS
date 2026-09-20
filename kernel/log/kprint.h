#pragma once

void kprint_init(void);

void kprint(const char *str);

void kprint_hex32(unsigned int value);

void kprint_hex64(unsigned long long value);

void kprint_dec_u64(unsigned long long value);

void kprint_hexdump(const void *addr, unsigned long size);
