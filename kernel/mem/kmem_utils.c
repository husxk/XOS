#include "mem/kmem_utils.h"

void *kmemset(void *s, int c, unsigned long n)
{
    unsigned char *p = s;
    unsigned char byte = (unsigned char)c;

    for (unsigned long i = 0; i < n; i++)
        p[i] = byte;

    return s;
}

void *kmemcpy(void *dst, const void *src, unsigned long n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;

    for (unsigned long i = 0; i < n; i++)
        d[i] = s[i];

    return dst;
}

unsigned long kstrlen(const char *s)
{
    unsigned long len = 0;

    while (s[len] != '\0')
        len++;

    return len;
}
