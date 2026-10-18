#pragma once

/*
 * Generic atomic operations.
 * They are part of the compiler, not a library: in -ffreestanding they
 * need no header and no linking (for the inline widths below), so they
 * cost nothing we do not already have.
 */

typedef enum atomic_memory_order
{
    MEMORY_ORDER_RELAXED = __ATOMIC_RELAXED,
    MEMORY_ORDER_ACQUIRE = __ATOMIC_ACQUIRE,
    MEMORY_ORDER_RELEASE = __ATOMIC_RELEASE,
    MEMORY_ORDER_ACQ_REL = __ATOMIC_ACQ_REL,
    MEMORY_ORDER_SEQ_CST = __ATOMIC_SEQ_CST,
} atomic_memory_order_t;

#define atomic_load(ptr, order)          __atomic_load_n((ptr), (order))
#define atomic_store(ptr, val, order)    __atomic_store_n((ptr), (val), (order))
#define atomic_exchange(ptr, val, order) __atomic_exchange_n((ptr), (val), (order))

#define atomic_compare_exchange(ptr, expected, desired, succ, fail) \
    __atomic_compare_exchange_n((ptr), (expected), (desired), 0, (succ), (fail))

#define atomic_fetch_add(ptr, val, order) __atomic_fetch_add((ptr), (val), (order))
#define atomic_fetch_sub(ptr, val, order) __atomic_fetch_sub((ptr), (val), (order))
#define atomic_fetch_or(ptr, val, order)  __atomic_fetch_or((ptr), (val), (order))
#define atomic_fetch_and(ptr, val, order) __atomic_fetch_and((ptr), (val), (order))

#define atomic_thread_fence(order)        __atomic_thread_fence((order))
