#include "mem/multiboot2.h"
#include "mem/phys_map.h"

#include "multiboot-mmap-fixture.hpp"

#include <gtest/gtest.h>

class PhysMapTest : public ::testing::Test
{
protected:
    alignas(8) unsigned char multiboot_bytes[8192];

    void TearDown() override
    {
        /* phys_map keeps regions in static storage; clear it between tests. */
        phys_map_init_from_info(0);
    }
};

TEST_F(PhysMapTest, NullInfoLeavesMapEmpty)
{
    unsigned int count;

    phys_map_init_from_info(0);
    count = phys_map_region_count();
    EXPECT_EQ(count, 0u);
}

TEST_F(PhysMapTest, IngestsSingleMmapEntry)
{
    const multiboot_mmap_entry entries[] = {
        {0x100000ULL, 0x200000ULL, MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
    };
    const multiboot_boot_info *info;
    const struct phys_region *region;
    unsigned int count;

    info = xos_test::multiboot_build_mmap_info(multiboot_bytes, sizeof(multiboot_bytes),
                                               entries, 1u, 0);
    ASSERT_NE(info, nullptr);

    phys_map_init_from_info(info);
    count = phys_map_region_count();
    EXPECT_EQ(count, 1u);

    region = phys_map_region(0);
    ASSERT_NE(region, nullptr);
    EXPECT_EQ(region->base, 0x100000ULL);
    EXPECT_EQ(region->length, 0x200000ULL);
    EXPECT_EQ(region->type, MULTIBOOT_MMAP_TYPE_AVAILABLE);
}

TEST_F(PhysMapTest, TotalAvailableSumsAvailRegionsOnly)
{
    const multiboot_mmap_entry entries[] = {
        {0x0ULL, 0x100000ULL, MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
        {0xf0000000ULL, 0x10000ULL, MULTIBOOT_MMAP_TYPE_RESERVED, 0u},
        {0x100000ULL, 0x400000ULL, MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
    };
    const multiboot_boot_info *info;
    unsigned long long total;

    info = xos_test::multiboot_build_mmap_info(multiboot_bytes, sizeof(multiboot_bytes),
                                               entries, 3u, 0);
    ASSERT_NE(info, nullptr);

    phys_map_init_from_info(info);
    total = phys_map_total_available();
    EXPECT_EQ(total, 0x100000ULL + 0x400000ULL);
}

TEST_F(PhysMapTest, ReinitReplacesPreviousRegions)
{
    const multiboot_mmap_entry first[] = {
        {0x1000ULL, 0x1000ULL, MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
    };
    const multiboot_mmap_entry second[] = {
        {0x2000ULL, 0x3000ULL, MULTIBOOT_MMAP_TYPE_RESERVED, 0u},
        {0x5000ULL, 0x1000ULL, MULTIBOOT_MMAP_TYPE_AVAILABLE, 0u},
    };
    const multiboot_boot_info *info;
    const struct phys_region *region0;
    const struct phys_region *region1;
    unsigned int count;

    info = xos_test::multiboot_build_mmap_info(multiboot_bytes, sizeof(multiboot_bytes),
                                               first, 1u, 0);
    ASSERT_NE(info, nullptr);
    phys_map_init_from_info(info);

    /* Reuse multiboot_bytes; second init must drop the 0x1000 region entirely. */
    info = xos_test::multiboot_build_mmap_info(multiboot_bytes, sizeof(multiboot_bytes),
                                               second, 2u, 0);
    ASSERT_NE(info, nullptr);
    phys_map_init_from_info(info);

    count = phys_map_region_count();
    EXPECT_EQ(count, 2u);

    region0 = phys_map_region(0);
    ASSERT_NE(region0, nullptr);
    EXPECT_EQ(region0->base, second[0].base_addr);
    EXPECT_EQ(region0->length, second[0].length);
    EXPECT_EQ(region0->type, second[0].type);

    region1 = phys_map_region(1);
    ASSERT_NE(region1, nullptr);
    EXPECT_EQ(region1->base, second[1].base_addr);
    EXPECT_EQ(region1->length, second[1].length);
    EXPECT_EQ(region1->type, second[1].type);
}

TEST_F(PhysMapTest, StopsAtRegionCap)
{
    multiboot_mmap_entry entries[PHYS_MAP_REGION_MAX + 1u];
    unsigned int i;
    const multiboot_boot_info *info;
    unsigned int count;

    for (i = 0; i < PHYS_MAP_REGION_MAX + 1u; i++)
    {
        entries[i].base_addr = (unsigned long long)i * 0x1000ULL;
        entries[i].length = 0x1000ULL;
        entries[i].type = MULTIBOOT_MMAP_TYPE_AVAILABLE;
        entries[i].reserved = 0u;
    }

    info = xos_test::multiboot_build_mmap_info(multiboot_bytes, sizeof(multiboot_bytes),
                                               entries, PHYS_MAP_REGION_MAX + 1u, 0);
    ASSERT_NE(info, nullptr);

    phys_map_init_from_info(info);
    count = phys_map_region_count();
    EXPECT_EQ(count, (unsigned int)PHYS_MAP_REGION_MAX);
}
