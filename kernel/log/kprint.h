#pragma once

#include <stdarg.h>

void kprint_init(void);

void kputs(const char *str);

void kprint(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void kvprint(const char *fmt, va_list ap) __attribute__((format(printf, 1, 0)));

void kprint_hexdump(const void *addr, unsigned long size);
