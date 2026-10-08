#include "mem/kmem_utils.h"

#include <gtest/gtest.h>

TEST(Kmemset, EmptyNoWrite)
{
    char buf[] = "xy";
    void *ret;

    ret = kmemset(buf, 'z', 0);
    EXPECT_EQ(ret, buf);
    EXPECT_STREQ(buf, "xy");
}

TEST(Kmemset, DoesNotWritePastCount)
{
    unsigned char buf[5];
    unsigned i;
    void *ret;

    for (i = 0; i < sizeof(buf); i++)
        buf[i] = 0x11u;

    ret = kmemset(buf, 0x22u, 2u);
    EXPECT_EQ(ret, buf);

    EXPECT_EQ(buf[0], 0x22u);
    EXPECT_EQ(buf[1], 0x22u);
    EXPECT_EQ(buf[2], 0x11u);
    EXPECT_EQ(buf[3], 0x11u);
    EXPECT_EQ(buf[4], 0x11u);
}

TEST(Kmemset, PartialFillAtOffset)
{
    unsigned char slab[6];
    unsigned i;
    void *ret;

    for (i = 0; i < sizeof(slab); i++)
        slab[i] = 0u;

    slab[0] = 0xa5u;
    slab[5] = 0x5au;

    ret = kmemset(slab + 1, 0xffu, 3u);
    EXPECT_EQ(ret, slab + 1);

    EXPECT_EQ(slab[0], 0xa5u);
    EXPECT_EQ(slab[1], 0xffu);
    EXPECT_EQ(slab[2], 0xffu);
    EXPECT_EQ(slab[3], 0xffu);
    EXPECT_EQ(slab[4], 0u);
    EXPECT_EQ(slab[5], 0x5au);
}

TEST(Kmemset, FillsBuffer)
{
    char buf[4];
    void *ret;
    unsigned i;

    ret = kmemset(buf, 0xab, sizeof(buf));
    EXPECT_EQ(ret, buf);

    for (i = 0; i < sizeof(buf); i++)
    {
        unsigned char byte = static_cast<unsigned char>(buf[i]);

        EXPECT_EQ(byte, 0xabu);
    }
}

TEST(Kmemset, TruncatesFillByteToUnsignedChar)
{
    unsigned char buf[2];
    void *ret;

    ret = kmemset(buf, 0x12abu, sizeof(buf));
    EXPECT_EQ(ret, buf);

    EXPECT_EQ(buf[0], 0xabu);
    EXPECT_EQ(buf[1], 0xabu);
}

TEST(Kstrlen, Empty)
{
    unsigned long len;

    len = kstrlen("");
    EXPECT_EQ(len, 0u);
}

TEST(Kstrlen, NonEmpty)
{
    unsigned long len;

    len = kstrlen("abc");
    EXPECT_EQ(len, 3u);
}

TEST(Kstrlen, StopsAtFirstNull)
{
    const char s[] = {'a', '\0', 'b', 'c', '\0'};
    unsigned long len;

    len = kstrlen(s);
    EXPECT_EQ(len, 1u);
}

TEST(Kstrlen, SingleCharacter)
{
    unsigned long len;

    len = kstrlen("x");
    EXPECT_EQ(len, 1u);
}
