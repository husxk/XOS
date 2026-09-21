#pragma once

void kprint_init(void);

void kputs(const char *str);

void kprint(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void kprint_hexdump(const void *addr, unsigned long size);
