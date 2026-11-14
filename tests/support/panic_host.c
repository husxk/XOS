#include "panic.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Host-test backend for the kernel's panic path. The real kernel halts
 * the CPU; under the host test runner we abort() instead so gtest death
 * tests can observe a failed kassert. ring_buffer.c and the other shared
 * kernel sources link against this in place of kernel/panic.c.
 */

void panic_begin(void)
{
}

void panic_halt(void)
{
    abort();
}

void kpanic(const char *file, int line, const char *msg)
{
    fprintf(stderr, "kpanic %s:%d: assertion failed: %s\n", file, line, msg);
    panic_halt();
}
