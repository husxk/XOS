#pragma once

/*
 * Single-producer / single-consumer ring buffer (FIFO) of fixed-size
 * elements.
*/
typedef struct ring_buffer
{
    unsigned char *buf;
    unsigned int  cap;
    unsigned int  elem_size;
    unsigned int  head;
    unsigned int  tail;
} ring_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

void ring_buffer_init(ring_buffer_t *rb, void *storage, unsigned int capacity,
                      unsigned int elem_size);

int ring_buffer_put(ring_buffer_t *rb, const void *elem);

int ring_buffer_get(ring_buffer_t *rb, void *out);

int ring_buffer_is_empty(const ring_buffer_t *rb);

int ring_buffer_is_full(const ring_buffer_t *rb);

unsigned int ring_buffer_count(const ring_buffer_t *rb);

unsigned int ring_buffer_capacity(const ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif
