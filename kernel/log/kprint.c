#include "log/kprint.h"

#include "drivers/vga/vga.h"
#include "mem/kmem.h"

static void kprint_dec_unsigned(unsigned long value)
{
    char buf[21];
    unsigned int i = 20;

    buf[i] = '\0';
    if (value == 0)
    {
        vga_putchar('0');
        return;
    }

    while (value > 0)
    {
        i--;
        buf[i] = (char)('0' + (value % 10));
        value /= 10;
    }

    vga_puts(buf + i);
}

static void kprint_hexdump_line(const char *hex_part, const char *ascii_part)
{
    unsigned long len = kstrlen(hex_part);

    vga_puts(hex_part);
    while (len < 48)
    {
        vga_putchar(' ');
        len++;
    }

    vga_puts("   ");
    vga_puts(ascii_part);
    vga_putchar('\n');
}

void kprint_init(void)
{
    vga_init();
}

void kprint(const char *str)
{
    vga_puts(str);
}

void kprint_dec_u64(unsigned long long value)
{
    char buf[21];
    unsigned int i = 20;

    buf[i] = '\0';
    if (value == 0)
    {
        kprint("0");
        return;
    }

    while (value > 0 && i > 0)
    {
        i--;
        buf[i] = (char)('0' + (value % 10));
        value /= 10;
    }

    kprint(buf + i);
}

void kprint_hex32(unsigned int value)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[11];
    unsigned int i;

    buf[0] = '0';
    buf[1] = 'x';

    for (i = 0; i < 8; i++)
    {
        unsigned int shift = (7 - i) * 4;
        buf[2 + i] = hex[(value >> shift) & 0xf];
    }

    buf[10] = '\0';
    kprint(buf);
}

void kprint_hex64(unsigned long long value)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[19];
    unsigned int i;

    buf[0] = '0';
    buf[1] = 'x';

    for (i = 0; i < 16; i++)
    {
        unsigned int shift = (15 - i) * 4;
        buf[2 + i] = hex[(value >> shift) & 0xf];
    }

    buf[18] = '\0';
    kprint(buf);
}

void kprint_hexdump(const void *addr, unsigned long size)
{
    const char hex_chars[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    };

    char dumpdata[16 * 3];
    char dumpdata2[17];

    const unsigned char *data;
    unsigned long p;

    if (addr == 0)
        return;

    data = addr;

    kprint("hexdump len ");
    kprint_dec_unsigned(size);
    kprint("\n");

    kmemset(dumpdata, 0, 16 * 3);
    kmemset(dumpdata2, 0, 17);

    p = 0;
    while (p < size)
    {
        unsigned char c = data[p];
        unsigned int index = (unsigned int)(p % 16);
        unsigned int offset = 3 * index;

        dumpdata[offset]     = hex_chars[(c >> 4) & 0x0f];
        dumpdata[offset + 1] = hex_chars[c & 0x0f];
        dumpdata[offset + 2] = ' ';

        dumpdata2[index] = (c >= 32 && c < 127) ? (char)c : '.';

        if ((index == 15) || (p == size - 1))
        {
            dumpdata[16 * 3 - 1] = '\0';
            dumpdata2[16] = '\0';
            kprint_hexdump_line(dumpdata, dumpdata2);

            kmemset(dumpdata, 0, 16 * 3);
            kmemset(dumpdata2, 0, 17);
        }

        p++;
    }
}
