#include "util/ring_buffer.h"

#include "cpu/atomic.h"
#include "mem/kmem_utils.h"

void ring_buffer_init(ring_buffer_t *rb, void *storage, unsigned int capacity,
                      unsigned int elem_size)
{
    rb->buf = storage;
    rb->cap = capacity;
    rb->elem_size = elem_size;

    atomic_store(&rb->head, 0u, MEMORY_ORDER_RELAXED);
    atomic_store(&rb->tail, 0u, MEMORY_ORDER_RELAXED);
}

int ring_buffer_put(ring_buffer_t *rb, const void *elem)
{
    /* The producer owns head. */
    unsigned int head = atomic_load(&rb->head, MEMORY_ORDER_RELAXED);
    unsigned int tail = atomic_load(&rb->tail, MEMORY_ORDER_ACQUIRE);
    unsigned int next = (head + 1u) % rb->cap;

    if (next == tail)
        return 0;

    kmemcpy(rb->buf + head * rb->elem_size, elem, rb->elem_size);

    atomic_store(&rb->head, next, MEMORY_ORDER_RELEASE);

    return 1;
}

int ring_buffer_get(ring_buffer_t *rb, void *out)
{
    /* The consumer owns tail. */
    unsigned int tail = atomic_load(&rb->tail, MEMORY_ORDER_RELAXED);
    unsigned int head = atomic_load(&rb->head, MEMORY_ORDER_ACQUIRE);

    if (tail == head)
        return 0;

    kmemcpy(out, rb->buf + tail * rb->elem_size, rb->elem_size);

    atomic_store(&rb->tail, (tail + 1u) % rb->cap, MEMORY_ORDER_RELEASE);

    return 1;
}

int ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return atomic_load(&rb->head, MEMORY_ORDER_RELAXED) ==
           atomic_load(&rb->tail, MEMORY_ORDER_RELAXED);
}

int ring_buffer_is_full(const ring_buffer_t *rb)
{
    unsigned int head = atomic_load(&rb->head, MEMORY_ORDER_RELAXED);
    unsigned int tail = atomic_load(&rb->tail, MEMORY_ORDER_RELAXED);

    return (head + 1u) % rb->cap == tail;
}

unsigned int ring_buffer_count(const ring_buffer_t *rb)
{
    unsigned int head = atomic_load(&rb->head, MEMORY_ORDER_RELAXED);
    unsigned int tail = atomic_load(&rb->tail, MEMORY_ORDER_RELAXED);

    return (head + rb->cap - tail) % rb->cap;
}

unsigned int ring_buffer_capacity(const ring_buffer_t *rb)
{
    return rb->cap - 1u;
}
