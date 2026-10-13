#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void *kmemset(void *s, int c, unsigned long n);

void *kmemcpy(void *dst, const void *src, unsigned long n);

unsigned long kstrlen(const char *s);

#ifdef __cplusplus
}
#endif
