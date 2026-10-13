#pragma once

/* Stored on the element when it is not in any heap. */
#define MIN_HEAP_IDX_NONE (-1)

/*
 * Compare heap keys for a min-heap (smallest key at index 0).
 * Return < 0 if a is ordered before b, 0 if equivalent, > 0 if a is after b
 */
typedef int (*min_heap_cmp_fn)(const void *a, const void *b);

/*
 * Called whenever an element's array index changes (including removal:
 * idx == MIN_HEAP_IDX_NONE). Required for min_heap_remove().
 */
typedef void (*min_heap_index_fn)(void *item, int idx);

typedef struct min_heap
{
    void **data;
    unsigned int size;
    unsigned int cap;
    min_heap_cmp_fn cmp;
    min_heap_index_fn set_index;
} min_heap_t;

#ifdef __cplusplus
extern "C" {
#endif

void min_heap_init(min_heap_t *heap, min_heap_cmp_fn cmp, min_heap_index_fn set_index);

void min_heap_free(min_heap_t *heap);

int min_heap_push(min_heap_t *heap, void *item);

void *min_heap_peek(const min_heap_t *heap);

void *min_heap_pop(min_heap_t *heap);

int min_heap_remove(min_heap_t *heap, int idx);

#ifdef __cplusplus
}
#endif
