#include "util/ring_buffer.h"

#include <gtest/gtest.h>

#include <thread>

namespace {

/*
 * Single-producer / single-consumer stress test. One thread pushes a long
 * ascending sequence, another drains it; the ring buffer's contract allows
 * exactly one of each. A small capacity against a large N forces the buffer
 * to fill and wrap many times, so index arithmetic, the full/empty checks,
 * and any lost or duplicated element get exercised under real contention.
 *
 * On x86 this passes even with the memory orders weakened to relaxed
 * (acquire/release and relaxed emit identical loads and stores there), so a
 * weakened order is only caught when this runs under ThreadSanitizer -- build
 * it with ./scripts/build.sh --sanitize.
 */
TEST(RingBufferConcurrency, SpscStressPreservesEveryItemInOrder)
{
    constexpr unsigned int Cap = 16u;
    constexpr int N = 1 << 20;

    alignas(int) unsigned char storage[Cap * sizeof(int)];
    ring_buffer_t rb;
    ring_buffer_init(&rb, storage, Cap, sizeof(int));

    std::thread producer([&] {
        for (int i = 0; i < N; ++i)
            while (ring_buffer_put(&rb, &i) == 0)
                /* spin until the consumer frees a slot */;
    });

    bool order_ok = true;
    int seen = 0;
    std::thread consumer([&] {
        for (int i = 0; i < N; ++i)
        {
            int out = -1;

            while (ring_buffer_get(&rb, &out) == 0)
                /* spin until the producer adds one */;

            if (out != i)
            {
                order_ok = false;
                break;
            }

            seen = i + 1;
        }
    });

    producer.join();
    consumer.join();

    EXPECT_TRUE(order_ok) << "FIFO order or payload integrity violated";
    EXPECT_EQ(seen, N);
    EXPECT_TRUE(ring_buffer_is_empty(&rb));
}

} /* namespace */
