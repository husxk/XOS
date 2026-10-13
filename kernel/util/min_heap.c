#include "util/min_heap.h"

#include "mem/kmem.h"

#define MIN_HEAP_INIT_CAP 8u

static unsigned int min_heap_parent(unsigned int i)
{
    return (i - 1u) / 2u;
}

static void min_heap_set_idx(min_heap_t *heap, void *item, int idx)
{
    if (heap->set_index != 0)
        heap->set_index(item, idx);
}

static void min_heap_swap(min_heap_t *heap, unsigned int a, unsigned int b)
{
    void *tmp = heap->data[a];

    heap->data[a] = heap->data[b];
    heap->data[b] = tmp;

    min_heap_set_idx(heap, heap->data[a], a);
    min_heap_set_idx(heap, heap->data[b], b);
}

static void min_heap_sift_up(min_heap_t *heap, unsigned int i)
{
    while (i > 0u)
    {
        unsigned int p = min_heap_parent(i);

        if (heap->cmp(heap->data[i], heap->data[p]) >= 0)
            break;

        min_heap_swap(heap, i, p);
        i = p;
    }
}

static void min_heap_sift_down(min_heap_t *heap, unsigned int i)
{
    for (;;)
    {
        unsigned int left = 2u * i + 1u;
        unsigned int right = left + 1u;
        unsigned int smallest = i;

        if (left < heap->size && heap->cmp(heap->data[left], heap->data[smallest]) < 0)
            smallest = left;

        if (right < heap->size && heap->cmp(heap->data[right], heap->data[smallest]) < 0)
            smallest = right;

        if (smallest == i)
            break;

        min_heap_swap(heap, i, smallest);
        i = smallest;
    }
}

static int min_heap_grow(min_heap_t *heap)
{
    unsigned int new_cap = heap->cap != 0u ? heap->cap * 2u : MIN_HEAP_INIT_CAP;
    void **nd = (void **)krealloc(heap->data, (unsigned long)new_cap * sizeof(void *));

    if (nd == 0)
        return -1;

    heap->data = nd;
    heap->cap = new_cap;
    return 0;
}

void min_heap_init(min_heap_t *heap, min_heap_cmp_fn cmp, min_heap_index_fn set_index)
{
    heap->data = 0;
    heap->size = 0;
    heap->cap = 0;
    heap->cmp = cmp;
    heap->set_index = set_index;
}

void min_heap_free(min_heap_t *heap)
{
    if (heap->data != 0)
    {
        kfree(heap->data);
        heap->data = 0;
    }

    heap->size = 0;
    heap->cap = 0;
}

int min_heap_push(min_heap_t *heap, void *item)
{
    if (heap->cmp == 0 || item == 0)
        return -1;

    if (heap->size >= heap->cap && min_heap_grow(heap) != 0)
        return -1;

    unsigned int i = heap->size;

    heap->data[i] = item;
    heap->size++;

    min_heap_set_idx(heap, item, (int)i);
    min_heap_sift_up(heap, i);

    return 0;
}

void *min_heap_peek(const min_heap_t *heap)
{
    if (heap->size == 0u)
        return 0;

    return heap->data[0];
}

void *min_heap_pop(min_heap_t *heap)
{
    if (heap->size == 0u)
        return 0;

    void *root = heap->data[0];
    unsigned int last = heap->size - 1u;

    heap->size = last;
    min_heap_set_idx(heap, root, MIN_HEAP_IDX_NONE);

    if (last == 0u)
    {
        heap->data[0] = 0;
        return root;
    }

    heap->data[0] = heap->data[last];
    heap->data[last] = 0;

    min_heap_set_idx(heap, heap->data[0], 0);
    min_heap_sift_down(heap, 0u);

    return root;
}

int min_heap_remove(min_heap_t *heap, int idx)
{
    if (idx < 0 || (unsigned int)idx >= heap->size)
        return -1;

    void *removed = heap->data[idx];
    unsigned int uidx = (unsigned int)idx;
    unsigned int last = heap->size - 1u;

    heap->size = last;
    min_heap_set_idx(heap, removed, MIN_HEAP_IDX_NONE);

    if (uidx == last)
    {
        heap->data[last] = 0;
        return 0;
    }

    heap->data[uidx] = heap->data[last];
    heap->data[last] = 0;

    min_heap_set_idx(heap, heap->data[uidx], idx);
    min_heap_sift_up(heap, uidx);
    min_heap_sift_down(heap, uidx);

    return 0;
}
