#include "mem/kmem.h"

void *kmemset(void *s, int c, unsigned long n)
{
    unsigned char *p = s;
    unsigned char byte = (unsigned char)c;

    for (unsigned long i = 0; i < n; i++)
        p[i] = byte;

    return s;
}

unsigned long kstrlen(const char *s)
{
    unsigned long len = 0;

    while (s[len] != '\0')
        len++;

    return len;
}
