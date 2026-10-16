#include "util/ring_buffer.h"

#include "mem/kmem_utils.h"

void ring_buffer_init(ring_buffer_t *rb, void *storage, unsigned int capacity,
                      unsigned int elem_size)
{
    rb->buf = storage;
    rb->cap = capacity;
    rb->elem_size = elem_size;
    rb->head = 0u;
    rb->tail = 0u;
}

int ring_buffer_put(ring_buffer_t *rb, const void *elem)
{
    unsigned int next = (rb->head + 1u) % rb->cap;

    if (next == rb->tail)
        return 0;

    kmemcpy(rb->buf + rb->head * rb->elem_size, elem, rb->elem_size);

    __asm__ volatile("" ::: "memory");
    rb->head = next;

    return 1;
}

int ring_buffer_get(ring_buffer_t *rb, void *out)
{
    if (rb->tail == rb->head)
        return 0;

    kmemcpy(out, rb->buf + rb->tail * rb->elem_size, rb->elem_size);

    __asm__ volatile("" ::: "memory");
    rb->tail = (rb->tail + 1u) % rb->cap;

    return 1;
}

int ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

int ring_buffer_is_full(const ring_buffer_t *rb)
{
    return (rb->head + 1u) % rb->cap == rb->tail;
}

unsigned int ring_buffer_count(const ring_buffer_t *rb)
{
    return (rb->head + rb->cap - rb->tail) % rb->cap;
}

unsigned int ring_buffer_capacity(const ring_buffer_t *rb)
{
    return rb->cap - 1u;
}
