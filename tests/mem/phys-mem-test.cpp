#include "mem/phys_map.h"
#include "mem/phys_mem.h"
#include "mem/phys_mem_err.h"

#include "phys-mem-fixture.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <sys/mman.h>

namespace {

/*
 * Identity "physical" base below 4 GiB — phys_mem ignores AVAIL regions at or
 * above PHYS_MEM_ADDR_CAP (32-bit kernel limit).
 */
constexpr uintptr_t FakePhysBase = 0x08000000u;
constexpr unsigned long FakeRamSize = 4u * 1024u * 1024u;

unsigned char *fake_phys_ram;

} /* namespace */

class PhysMemTest : public ::testing::Test
{
protected:
    alignas(8) unsigned char multiboot_bytes[8192];

    struct phys_mem_reserved_range page0_reserve_;

    static void SetUpTestSuite()
    {
        fake_phys_ram = (unsigned char *)mmap((void *)(uintptr_t)FakePhysBase, FakeRamSize,
                                              PROT_READ | PROT_WRITE,
                                              MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
        if (fake_phys_ram == MAP_FAILED)
            fake_phys_ram = 0;
    }

    static void TearDownTestSuite()
    {
        if (fake_phys_ram != 0)
        {
            munmap(fake_phys_ram, FakeRamSize);
            fake_phys_ram = 0;
        }
    }

    void SetUp() override
    {
        int err;

        if (fake_phys_ram == 0)
            GTEST_SKIP() << "MAP_FIXED mmap at FakePhysBase failed";

        page0_reserve_.base = 0;
        page0_reserve_.length = 4096u;

        err = xos_test::phys_mem_test_init_from_ram(
            fake_phys_ram, FakeRamSize, multiboot_bytes, sizeof(multiboot_bytes),
            &page0_reserve_, 1u);
        ASSERT_EQ(err, PHYS_MEM_OK);
    }

    void TearDown() override
    {
        phys_map_init_from_info(0);
    }
};

TEST_F(PhysMemTest, InitReportsFramesAndFreeMemory)
{
    unsigned long frame_total = 0;
    unsigned long free_count = 0;
    unsigned long bitmap_size_bytes = 0;

    phys_mem_stats(&frame_total, &free_count, &bitmap_size_bytes);

    EXPECT_GT(frame_total, 0u);
    EXPECT_GT(free_count, 0u);
    EXPECT_GT(bitmap_size_bytes, 0u);
    EXPECT_LT(free_count, frame_total);
}

TEST_F(PhysMemTest, AllocPageReturnsAlignedNonNull)
{
    void *page;
    unsigned long phys;

    page = phys_mem_alloc_page();
    ASSERT_NE(page, nullptr);

    phys = (unsigned long)(uintptr_t)page;
    EXPECT_EQ(phys & (4096u - 1u), 0u);
    EXPECT_NE(phys, 0u);
}

TEST_F(PhysMemTest, FreeReturnsOk)
{
    void *page;
    int err;

    page = phys_mem_alloc_page();
    ASSERT_NE(page, nullptr);

    err = phys_mem_free_page(page);
    EXPECT_EQ(err, PHYS_MEM_OK);
}

TEST_F(PhysMemTest, FreeUnalignedReturnsEinval)
{
    unsigned char *bogus = fake_phys_ram + 1u;
    int err;

    err = phys_mem_free_page(bogus);
    EXPECT_EQ(err, PHYS_MEM_EINVAL);
}

TEST_F(PhysMemTest, DoubleFreeReturnsEstate)
{
    void *page;
    int err;

    page = phys_mem_alloc_page();
    ASSERT_NE(page, nullptr);

    err = phys_mem_free_page(page);
    ASSERT_EQ(err, PHYS_MEM_OK);

    err = phys_mem_free_page(page);
    EXPECT_EQ(err, PHYS_MEM_ESTATE);
}

TEST_F(PhysMemTest, AllocFourPagesThenFree)
{
    void *block;
    int err;

    block = phys_mem_alloc_pages(4u);
    ASSERT_NE(block, nullptr);

    err = phys_mem_free_pages(block, 4u);
    EXPECT_EQ(err, PHYS_MEM_OK);
}

TEST_F(PhysMemTest, AllocZeroPagesReturnsNull)
{
    void *block;

    block = phys_mem_alloc_pages(0u);
    EXPECT_EQ(block, nullptr);
}

TEST_F(PhysMemTest, FreeZeroPageCountReturnsEinval)
{
    void *page;
    int err;

    page = phys_mem_alloc_page();
    ASSERT_NE(page, nullptr);

    err = phys_mem_free_pages(page, 0u);
    EXPECT_EQ(err, PHYS_MEM_EINVAL);

    EXPECT_EQ(phys_mem_free_page(page), PHYS_MEM_OK);
}

TEST_F(PhysMemTest, FreeUnusedFrameReturnsEstate)
{
    void *never_allocated;
    int err;

    never_allocated = (void *)(uintptr_t)(FakePhysBase + 4096u * 64u);
    err = phys_mem_free_page(never_allocated);
    EXPECT_EQ(err, PHYS_MEM_ESTATE);
}

TEST_F(PhysMemTest, ExhaustFreeListThenRecover)
{
    std::vector<void *> pages;
    void *page;
    unsigned long free_count = 0;

    while ((page = phys_mem_alloc_page()) != nullptr)
        pages.push_back(page);

    ASSERT_FALSE(pages.empty());

    page = phys_mem_alloc_page();
    EXPECT_EQ(page, nullptr);

    page = phys_mem_alloc_pages(1u);
    EXPECT_EQ(page, nullptr);

    phys_mem_stats(0, &free_count, 0);
    EXPECT_EQ(free_count, 0u);

    for (void *held : pages)
        EXPECT_EQ(phys_mem_free_page(held), PHYS_MEM_OK);

    page = phys_mem_alloc_page();
    ASSERT_NE(page, nullptr);
    EXPECT_EQ(phys_mem_free_page(page), PHYS_MEM_OK);
}

TEST_F(PhysMemTest, ManyDistinctAllocations)
{
    std::vector<void *> pages;

    for (unsigned int i = 0; i < 32u; i++)
    {
        void *page = phys_mem_alloc_page();

        ASSERT_NE(page, nullptr);
        pages.push_back(page);
    }

    for (unsigned int i = 0; i < pages.size(); i++)
    {
        for (unsigned int j = i + 1u; j < pages.size(); j++)
            EXPECT_NE(pages[i], pages[j]);
    }

    for (void *page : pages)
        EXPECT_EQ(phys_mem_free_page(page), PHYS_MEM_OK);
}

TEST_F(PhysMemTest, InterleavedAllocAndFree)
{
    void *slots[12];

    for (unsigned int i = 0; i < 12u; i++)
    {
        slots[i] = phys_mem_alloc_page();
        ASSERT_NE(slots[i], nullptr);
    }

    for (unsigned int i = 0; i < 12u; i += 2u)
        EXPECT_EQ(phys_mem_free_page(slots[i]), PHYS_MEM_OK);

    for (unsigned int i = 0; i < 6u; i++)
    {
        void *page = phys_mem_alloc_page();

        ASSERT_NE(page, nullptr);
        EXPECT_EQ(phys_mem_free_page(page), PHYS_MEM_OK);
    }

    for (unsigned int i = 1u; i < 12u; i += 2u)
        EXPECT_EQ(phys_mem_free_page(slots[i]), PHYS_MEM_OK);
}

TEST_F(PhysMemTest, AllocAfterFreeSucceeds)
{
    void *first;
    void *second;

    first = phys_mem_alloc_page();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(phys_mem_free_page(first), PHYS_MEM_OK);

    second = phys_mem_alloc_page();
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(phys_mem_free_page(second), PHYS_MEM_OK);
}

TEST_F(PhysMemTest, MultiPageAllocTooLargeReturnsNull)
{
    void *block;
    unsigned long frame_total = 0;

    phys_mem_stats(&frame_total, 0, 0);
    ASSERT_GT(frame_total, 0u);

    block = phys_mem_alloc_pages(frame_total + 1u);
    EXPECT_EQ(block, nullptr);
}

TEST(PhysMemInit, EmptyMapReturnsEnomap)
{
    struct phys_mem_init_params params;
    int err;

    phys_map_init_from_info(0);

    params.bitmap_min_phys = FakePhysBase;
    params.reserved = 0;
    params.reserved_count = 0;

    err = phys_mem_init_from_params(&params);
    EXPECT_EQ(err, PHYS_MEM_ENOMAP);
}
