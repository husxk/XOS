#pragma once

#define KTHREAD_STACK_SIZE 4096u
#define KTHREAD_MAX 32u

#define KTHREAD_ID_NONE (-1)

#ifdef __cplusplus
extern "C" {
#endif

void kthread_init(void);

int kthread_create(void (*fn)(void *arg), void *arg);

void kthread_yield(void);

void kthread_run(void);

__attribute__((noreturn)) void kthread_exit(void);

#ifdef __cplusplus
}
#endif
