#include "util/ring_buffer.h"

#include <gtest/gtest.h>

namespace {

constexpr unsigned int DefaultCapacity = 8u;
constexpr unsigned long StorageBytes = 1024u;

struct Packet
{
    int key;
    unsigned int flags;
    char tag[4];
};

bool packet_eq(const Packet &a, const Packet &b)
{
    return a.key == b.key && a.flags == b.flags &&
           a.tag[0] == b.tag[0] && a.tag[1] == b.tag[1] &&
           a.tag[2] == b.tag[2] && a.tag[3] == b.tag[3];
}

} /* namespace */

class RingBufferTest : public ::testing::Test
{
protected:
    alignas(8) unsigned char storage[StorageBytes];
    ring_buffer_t rb;

    void SetUp() override
    {
        ring_buffer_init(&rb, storage, DefaultCapacity, sizeof(int));
    }

    int put(int value)
    {
        return ring_buffer_put(&rb, &value);
    }

    int get(int *out)
    {
        return ring_buffer_get(&rb, out);
    }
};

TEST_F(RingBufferTest, FreshBufferIsEmpty)
{
    EXPECT_TRUE(ring_buffer_is_empty(&rb));
    EXPECT_FALSE(ring_buffer_is_full(&rb));
    EXPECT_EQ(ring_buffer_count(&rb), 0u);
}

TEST_F(RingBufferTest, CapacityIsOneLessThanBacking)
{
    EXPECT_EQ(ring_buffer_capacity(&rb), DefaultCapacity - 1u);

    ring_buffer_init(&rb, storage, 16u, sizeof(int));
    EXPECT_EQ(ring_buffer_capacity(&rb), 15u);

    ring_buffer_init(&rb, storage, 2u, sizeof(int));
    EXPECT_EQ(ring_buffer_capacity(&rb), 1u);
}

TEST_F(RingBufferTest, GetFromEmptyReturnsZeroAndLeavesOutput)
{
    int out = 0x1234;

    EXPECT_EQ(ring_buffer_get(&rb, &out), 0);
    EXPECT_EQ(out, 0x1234);
}

TEST_F(RingBufferTest, SinglePutThenGet)
{
    ASSERT_EQ(put(42), 1);
    EXPECT_EQ(ring_buffer_count(&rb), 1u);
    EXPECT_FALSE(ring_buffer_is_empty(&rb));

    int out = 0;

    ASSERT_EQ(get(&out), 1);
    EXPECT_EQ(out, 42);
    EXPECT_EQ(ring_buffer_count(&rb), 0u);
    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

TEST_F(RingBufferTest, PreservesFifoOrder)
{
    const int values[] = {10, 20, 30, 40, 50};

    for (int v : values)
        ASSERT_EQ(put(v), 1);

    EXPECT_EQ(ring_buffer_count(&rb), 5u);

    for (int expected : values)
    {
        int out = 0;

        ASSERT_EQ(get(&out), 1);
        EXPECT_EQ(out, expected);
    }

    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

TEST_F(RingBufferTest, FillsToUsableCapacityThenRejects)
{
    const unsigned int usable = ring_buffer_capacity(&rb);

    for (unsigned int i = 0; i < usable; i++)
        ASSERT_EQ(put(static_cast<int>(i)), 1) << "put #" << i << " should fit";

    EXPECT_TRUE(ring_buffer_is_full(&rb));
    EXPECT_EQ(ring_buffer_count(&rb), usable);

    int extra = 999;

    EXPECT_EQ(ring_buffer_put(&rb, &extra), 0);
    EXPECT_EQ(ring_buffer_count(&rb), usable);

    for (unsigned int i = 0; i < usable; i++)
    {
        int out = -1;

        ASSERT_EQ(get(&out), 1);
        EXPECT_EQ(out, static_cast<int>(i));
    }

    int drained = 0;

    EXPECT_EQ(get(&drained), 0);
    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

TEST_F(RingBufferTest, CountReflectsOutstandingElements)
{
    EXPECT_EQ(ring_buffer_count(&rb), 0u);

    ASSERT_EQ(put(1), 1);
    ASSERT_EQ(put(2), 1);
    ASSERT_EQ(put(3), 1);
    EXPECT_EQ(ring_buffer_count(&rb), 3u);

    int out = 0;

    ASSERT_EQ(get(&out), 1);
    EXPECT_EQ(ring_buffer_count(&rb), 2u);

    ASSERT_EQ(get(&out), 1);
    ASSERT_EQ(get(&out), 1);
    EXPECT_EQ(ring_buffer_count(&rb), 0u);
}

TEST_F(RingBufferTest, WrapsAroundPreservingOrder)
{
    ring_buffer_init(&rb, storage, 4u, sizeof(int));

    ASSERT_EQ(put(1), 1);
    ASSERT_EQ(put(2), 1);
    ASSERT_EQ(put(3), 1);
    ASSERT_TRUE(ring_buffer_is_full(&rb));

    int out = 0;

    ASSERT_EQ(get(&out), 1);
    EXPECT_EQ(out, 1);
    ASSERT_EQ(get(&out), 1);
    EXPECT_EQ(out, 2);

    ASSERT_EQ(put(4), 1);
    ASSERT_EQ(put(5), 1);
    ASSERT_TRUE(ring_buffer_is_full(&rb));

    const int expected[] = {3, 4, 5};

    for (int e : expected)
    {
        ASSERT_EQ(get(&out), 1);
        EXPECT_EQ(out, e);
    }

    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

TEST_F(RingBufferTest, HandlesArbitraryElementSize)
{
    ring_buffer_init(&rb, storage, 4u, sizeof(Packet));

    const Packet in[] = {
        {1, 0x10u, {'a', 'b', 'c', 0}},
        {2, 0x20u, {'d', 'e', 'f', 0}},
        {3, 0x30u, {'g', 'h', 'i', 0}},
    };

    for (const Packet &p : in)
        ASSERT_EQ(ring_buffer_put(&rb, &p), 1);

    for (const Packet &expected : in)
    {
        Packet out{};

        ASSERT_EQ(ring_buffer_get(&rb, &out), 1);
        EXPECT_TRUE(packet_eq(out, expected));
    }

    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

TEST_F(RingBufferTest, ReinitResetsState)
{
    ASSERT_EQ(put(7), 1);
    ASSERT_EQ(put(8), 1);
    ASSERT_FALSE(ring_buffer_is_empty(&rb));

    ring_buffer_init(&rb, storage, DefaultCapacity, sizeof(int));

    EXPECT_TRUE(ring_buffer_is_empty(&rb));
    EXPECT_EQ(ring_buffer_count(&rb), 0u);

    int out = 0;

    EXPECT_EQ(get(&out), 0);
}

TEST_F(RingBufferTest, FullAndEmptyFlagsAtBoundaries)
{
    const unsigned int usable = ring_buffer_capacity(&rb);

    for (unsigned int i = 0; i < usable; i++)
    {
        EXPECT_FALSE(ring_buffer_is_full(&rb));
        ASSERT_EQ(put(static_cast<int>(i)), 1);
        EXPECT_FALSE(ring_buffer_is_empty(&rb));
    }

    EXPECT_TRUE(ring_buffer_is_full(&rb));
}
