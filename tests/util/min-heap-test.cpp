#include "util/min_heap.h"

#include "mem/kmem.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr unsigned long TestArenaSize = 4096u;

struct MinHeapItem
{
    int key;
    int heap_idx;
};

int min_heap_item_cmp(const void *a, const void *b)
{
    const MinHeapItem *ia = static_cast<const MinHeapItem *>(a);
    const MinHeapItem *ib = static_cast<const MinHeapItem *>(b);

    if (ia->key < ib->key)
        return -1;

    if (ia->key > ib->key)
        return 1;

    return 0;
}

void min_heap_item_set_index(void *item, int idx)
{
    static_cast<MinHeapItem *>(item)->heap_idx = idx;
}

int init_test_kheap(unsigned char *arena, unsigned long size)
{
    struct kheap_init_params params;

    params.arena = arena;
    params.size = size;
    return kheap_init_from_params(&params);
}

int min_of_remaining(const std::vector<MinHeapItem *> &live)
{
    int best = live.front()->key;

    for (MinHeapItem *p : live)
    {
        if (p->key < best)
            best = p->key;
    }

    return best;
}

} /* namespace */

class MinHeapTest : public ::testing::Test
{
protected:
    alignas(8) unsigned char arena[TestArenaSize];
    min_heap_t heap;

    void SetUp() override
    {
        ASSERT_EQ(init_test_kheap(arena, sizeof(arena)), KHEAP_OK);
        min_heap_init(&heap, min_heap_item_cmp, min_heap_item_set_index);
    }

    void TearDown() override
    {
        min_heap_free(&heap);
    }
};

TEST_F(MinHeapTest, PeekPopEmptyReturnNull)
{
    EXPECT_EQ(min_heap_peek(&heap), nullptr);
    EXPECT_EQ(min_heap_pop(&heap), nullptr);
}

TEST_F(MinHeapTest, PushRejectsNullItemOrCmp)
{
    MinHeapItem item{1, MIN_HEAP_IDX_NONE};

    EXPECT_EQ(min_heap_push(&heap, nullptr), -1);

    min_heap_init(&heap, nullptr, min_heap_item_set_index);
    EXPECT_EQ(min_heap_push(&heap, &item), -1);
}

TEST_F(MinHeapTest, SinglePushPopUpdatesIndex)
{
    MinHeapItem item{42, MIN_HEAP_IDX_NONE};

    ASSERT_EQ(min_heap_push(&heap, &item), 0);
    EXPECT_EQ(item.heap_idx, 0);
    EXPECT_EQ(min_heap_peek(&heap), &item);

    MinHeapItem *popped = static_cast<MinHeapItem *>(min_heap_pop(&heap));
    EXPECT_EQ(popped, &item);
    EXPECT_EQ(item.heap_idx, MIN_HEAP_IDX_NONE);
    EXPECT_EQ(min_heap_peek(&heap), nullptr);
}

TEST_F(MinHeapTest, MultiPushPopYieldsAscendingKeys)
{
    MinHeapItem items[] = {
        {5, MIN_HEAP_IDX_NONE},
        {1, MIN_HEAP_IDX_NONE},
        {3, MIN_HEAP_IDX_NONE},
        {2, MIN_HEAP_IDX_NONE},
        {4, MIN_HEAP_IDX_NONE},
    };

    for (MinHeapItem &it : items)
        ASSERT_EQ(min_heap_push(&heap, &it), 0);

    int prev = -1;

    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); i++)
    {
        MinHeapItem *top = static_cast<MinHeapItem *>(min_heap_peek(&heap));

        ASSERT_NE(top, nullptr);
        if (prev >= 0)
            EXPECT_GE(top->key, prev);

        prev = top->key;
        MinHeapItem *popped = static_cast<MinHeapItem *>(min_heap_pop(&heap));

        EXPECT_EQ(popped->key, top->key);
        EXPECT_EQ(popped->heap_idx, MIN_HEAP_IDX_NONE);
    }

    EXPECT_EQ(min_heap_pop(&heap), nullptr);
}

TEST_F(MinHeapTest, PeekMatchesMinOfLiveSetAfterEachPush)
{
    std::vector<MinHeapItem *> live;
    MinHeapItem items[] = {
        {17, MIN_HEAP_IDX_NONE},
        {3, MIN_HEAP_IDX_NONE},
        {99, MIN_HEAP_IDX_NONE},
        {3, MIN_HEAP_IDX_NONE},
        {0, MIN_HEAP_IDX_NONE},
        {42, MIN_HEAP_IDX_NONE},
    };
    const int push_order[] = {0, 2, 1, 5, 3, 4};

    for (int ord : push_order)
    {
        MinHeapItem *it = &items[ord];

        ASSERT_EQ(min_heap_push(&heap, it), 0);
        live.push_back(it);

        MinHeapItem *top = static_cast<MinHeapItem *>(min_heap_peek(&heap));

        ASSERT_NE(top, nullptr);
        EXPECT_EQ(top->key, min_of_remaining(live));
    }
}

TEST_F(MinHeapTest, RemoveRootMiddleAndLast)
{
    MinHeapItem items[] = {
        {10, MIN_HEAP_IDX_NONE},
        {20, MIN_HEAP_IDX_NONE},
        {30, MIN_HEAP_IDX_NONE},
        {40, MIN_HEAP_IDX_NONE},
        {50, MIN_HEAP_IDX_NONE},
    };

    for (MinHeapItem &it : items)
        ASSERT_EQ(min_heap_push(&heap, &it), 0);

    ASSERT_EQ(min_heap_remove(&heap, 0), 0);
    EXPECT_EQ(items[0].heap_idx, MIN_HEAP_IDX_NONE);
    EXPECT_EQ(static_cast<MinHeapItem *>(min_heap_peek(&heap))->key, 20);

    int idx_40 = items[3].heap_idx;

    ASSERT_GE(idx_40, 0);
    ASSERT_EQ(min_heap_remove(&heap, idx_40), 0);
    EXPECT_EQ(items[3].heap_idx, MIN_HEAP_IDX_NONE);

    int idx_50 = items[4].heap_idx;

    ASSERT_GE(idx_50, 0);
    ASSERT_EQ(min_heap_remove(&heap, idx_50), 0);
    EXPECT_EQ(items[4].heap_idx, MIN_HEAP_IDX_NONE);

    EXPECT_EQ(static_cast<MinHeapItem *>(min_heap_peek(&heap))->key, 20);
    EXPECT_EQ(min_heap_pop(&heap), &items[1]);
    EXPECT_EQ(min_heap_pop(&heap), &items[2]);
    EXPECT_EQ(min_heap_pop(&heap), nullptr);
}

TEST_F(MinHeapTest, RemoveRejectsInvalidIndex)
{
    MinHeapItem item{1, MIN_HEAP_IDX_NONE};

    ASSERT_EQ(min_heap_push(&heap, &item), 0);

    EXPECT_EQ(min_heap_remove(&heap, -1), -1);
    EXPECT_EQ(min_heap_remove(&heap, 1), -1);
}

TEST_F(MinHeapTest, GrowsPastInitialCapacity)
{
    MinHeapItem items[20];

    for (unsigned i = 0; i < sizeof(items) / sizeof(items[0]); i++)
    {
        items[i].key = static_cast<int>(19 - i);
        items[i].heap_idx = MIN_HEAP_IDX_NONE;
        ASSERT_EQ(min_heap_push(&heap, &items[i]), 0);
    }

    for (int expect = 0; expect < 20; expect++)
    {
        MinHeapItem *top = static_cast<MinHeapItem *>(min_heap_peek(&heap));

        ASSERT_NE(top, nullptr);
        EXPECT_EQ(top->key, expect);
        min_heap_pop(&heap);
    }
}

TEST_F(MinHeapTest, FreeThenReuse)
{
    MinHeapItem a{7, MIN_HEAP_IDX_NONE};
    MinHeapItem b{2, MIN_HEAP_IDX_NONE};

    ASSERT_EQ(min_heap_push(&heap, &a), 0);
    min_heap_free(&heap);

    min_heap_init(&heap, min_heap_item_cmp, min_heap_item_set_index);
    ASSERT_EQ(min_heap_push(&heap, &b), 0);
    EXPECT_EQ(min_heap_peek(&heap), &b);
}
