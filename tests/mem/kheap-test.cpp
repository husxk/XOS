#include "mem/kheap_err.h"
#include "mem/kmem.h"

#include "kheap-fixture.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr unsigned long GeneralArenaSize = 4096u;
constexpr unsigned long TightArenaSize = 256u;

/*
 * Sized so two kmalloc(80) blocks leave only a minimal tail free; a third
 * equal request fails until both are freed and coalesced.
 */
constexpr unsigned long CoalesceArenaSize = 256u;
constexpr unsigned long CoalesceChunkUserBytes = 80u;

} /* namespace */

TEST(KheapInitFromParams, RejectsNullParams)
{
    EXPECT_EQ(kheap_init_from_params(nullptr), KHEAP_EINVAL);
}

TEST(KheapInitFromParams, RejectsNullArena)
{
    struct kheap_init_params params;

    params.arena = nullptr;
    params.size = GeneralArenaSize;
    EXPECT_EQ(kheap_init_from_params(&params), KHEAP_EINVAL);
}

TEST(KheapInitFromParams, RejectsArenaTooSmall)
{
    alignas(8) unsigned char arena[8];

    EXPECT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_EINVAL);
}

TEST(KheapInitFromParams, RejectsMisalignedArena)
{
    alignas(8) unsigned char storage[GeneralArenaSize + 8];
    unsigned char *misaligned = storage + 1u;

    EXPECT_EQ(xos_test::kheap_test_init_from_arena(misaligned, GeneralArenaSize),
              KHEAP_EINVAL);
}

TEST(Kheap, KmallocZeroReturnsNull)
{
    alignas(8) unsigned char arena[GeneralArenaSize];

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);
    EXPECT_EQ(kmalloc(0), nullptr);
}

TEST(Kheap, KfreeNullNoCrash)
{
    alignas(8) unsigned char arena[GeneralArenaSize];

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);
    kfree(nullptr);
}

TEST(Kheap, AllocFreeReuse)
{
    alignas(8) unsigned char arena[GeneralArenaSize];
    void *first;
    void *second;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    first = kmalloc(64);
    ASSERT_NE(first, nullptr);

    kfree(first);

    second = kmalloc(64);
    EXPECT_NE(second, nullptr);
    kfree(second);
}

TEST(Kheap, SplitAllowsSecondAlloc)
{
    alignas(8) unsigned char arena[GeneralArenaSize];
    void *a;
    void *b;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    a = kmalloc(32);
    ASSERT_NE(a, nullptr);

    b = kmalloc(32);
    EXPECT_NE(b, nullptr);
    EXPECT_NE(a, b);

    kfree(a);
    kfree(b);
}

TEST(Kheap, AdjacentCoalesce)
{
    alignas(8) unsigned char arena[CoalesceArenaSize];
    void *a;
    void *b;
    void *blocked;
    void *merged;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    a = kmalloc(CoalesceChunkUserBytes);
    b = kmalloc(CoalesceChunkUserBytes);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    blocked = kmalloc(CoalesceChunkUserBytes);
    EXPECT_EQ(blocked, nullptr);

    kfree(a);
    kfree(b);

    merged = kmalloc(CoalesceChunkUserBytes);
    ASSERT_NE(merged, nullptr);
    kfree(merged);
}

TEST(Kheap, OomWhenArenaFull)
{
    alignas(8) unsigned char arena[TightArenaSize];
    std::vector<void *> live;
    void *recovered;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    while (void *ptr = kmalloc(16))
        live.push_back(ptr);

    ASSERT_GT(live.size(), 0u);
    EXPECT_EQ(kmalloc(16), nullptr);
    EXPECT_EQ(kmalloc(1), nullptr);

    kfree(live.front());
    live.erase(live.begin());

    recovered = kmalloc(16);
    ASSERT_NE(recovered, nullptr);
    live.push_back(recovered);

    for (void *ptr : live)
        kfree(ptr);
}

TEST(Kheap, DoesNotReuseLiveBlock)
{
    alignas(8) unsigned char arena[TightArenaSize];
    std::vector<void *> held;
    void *extra;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    held.push_back(kmalloc(96));
    ASSERT_NE(held.front(), nullptr);

    while ((extra = kmalloc(8)) != nullptr)
    {
        EXPECT_NE(extra, held.front());
        held.push_back(extra);
    }

    EXPECT_EQ(kmalloc(8), nullptr);

    for (void *ptr : held)
        kfree(ptr);
}

TEST(Kheap, KfreeOutOfArenaIsSilent)
{
    alignas(8) unsigned char arena[GeneralArenaSize];
    alignas(8) unsigned char outside[64];
    void *inside;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    inside = kmalloc(32);
    ASSERT_NE(inside, nullptr);

    kfree(outside);
    kfree(inside);
}

TEST(Kheap, KzallocZeroFill)
{
    alignas(8) unsigned char arena[GeneralArenaSize];
    unsigned char *bytes;
    unsigned i;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    bytes = static_cast<unsigned char *>(kzalloc(32));
    ASSERT_NE(bytes, nullptr);

    for (i = 0; i < 32u; i++)
        EXPECT_EQ(bytes[i], 0u);

    kfree(bytes);
}

/*
 * Current behavior: second kfree on the same pointer is a silent no-op
 * (block already marked free). Not a supported API guarantee.
 */
TEST(Kheap, DoubleFreeIsSilentNoOp)
{
    alignas(8) unsigned char arena[GeneralArenaSize];
    void *ptr;
    void *again;

    ASSERT_EQ(xos_test::kheap_test_init_from_arena(arena, sizeof(arena)), KHEAP_OK);

    ptr = kmalloc(48);
    ASSERT_NE(ptr, nullptr);

    kfree(ptr);
    kfree(ptr);

    again = kmalloc(48);
    EXPECT_NE(again, nullptr);
    kfree(again);
}
