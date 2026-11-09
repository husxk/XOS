#pragma once

/* Print the panic banner. */
void panic_begin(void);

/* Stop the CPU for good. Never returns. */
__attribute__((noreturn))
void panic_halt(void);

/* Report a failed invariant (file, line, expression), then halt. */
__attribute__((noreturn))
void kpanic(const char *file, int line, const char *msg);

#define kassert(cond) \
    ((cond) ? (void)0 : kpanic(__FILE__, __LINE__, #cond))
