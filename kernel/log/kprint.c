#include "log/kprint.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "drivers/vga/vga.h"
#include "mem/kmem.h"

#define KPRINT_WIDTH_MAX 256

enum kprint_state
{
    KPRINT_STATE_NORMAL,
    KPRINT_STATE_PERCENT,
    KPRINT_STATE_WIDTH,
    KPRINT_STATE_LENGTH,
};

enum kprint_length
{
    KPRINT_LENGTH_NONE,
    KPRINT_LENGTH_LONG,
    KPRINT_LENGTH_LONG_LONG,
    KPRINT_LENGTH_SIZE,
};

static void kprint_spec_literal(const char *start, const char *end)
{
    const char *p;

    for (p = start; p <= end; p++)
        vga_putchar(*p);
}

/* Argument order: value, base, upper, negative, width, pad. */
static void kprint_number_format(uint64_t value, unsigned int base, bool upper,
                                 bool negative, unsigned int width, char pad)
{
    static const char lower_digits[] = "0123456789abcdef";
    static const char upper_digits[] = "0123456789ABCDEF";
    const char *digits = upper ? upper_digits : lower_digits;
    char buf[sizeof(uint64_t) * 8];
    unsigned int len = 0;
    unsigned int total;

    do
    {
        buf[len] = digits[value % base];
        len++;
        value /= base;
    } while (value > 0);

    /*
    * The sign goes before zero padding but after space padding. Zeros
    * are part of the number, so the sign must lead then; spaces are
    * only alignment, so the sign stays attached to the digits. Exactly
    * one of the two sign checks around the padding loop fires.
    */

    total = negative ? len + 1 : len;

    if (negative && pad == '0')
        vga_putchar('-');

    /*
    * Pad up to the requested width. This compares instead of computing
    * width - total, because both are unsigned and the subtraction would
    * wrap around when the number is wider than the field. A wider
    * number is printed in full; the width is a minimum, not a limit.
    * The sign is already counted in total, so width never needs to be
    * adjusted for it.
    */

    while (width > total)
    {
        vga_putchar(pad);
        width--;
    }

    if (negative && pad != '0')
        vga_putchar('-');

    /*
    * Digits were produced least significant first, so buf holds them
    * in reverse order. Print from the end.
    */

    while (len > 0)
    {
        len--;
        vga_putchar(buf[len]);
    }
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

void kputs(const char *str)
{
    vga_puts(str);
}

static void kvprint(const char *fmt, va_list ap);

void kprint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    kvprint(fmt, ap);
    va_end(ap);
}

static void kvprint(const char *fmt, va_list ap)
{
    enum kprint_state state = KPRINT_STATE_NORMAL;
    const char *spec_start = 0;
    unsigned int width = 0;
    char pad = ' ';
    const char *s;
    unsigned long len;
    unsigned int base;
    uint64_t value;
    long long signed_value;
    bool negative;
    enum kprint_length length = KPRINT_LENGTH_NONE;
    char ch;

    while (*fmt != '\0')
    {
        char c = *fmt;

        switch (state)
        {
            case KPRINT_STATE_NORMAL:
                if (c == '%')
                {
                    spec_start = fmt;
                    width = 0;
                    pad = ' ';
                    length = KPRINT_LENGTH_NONE;
                    state = KPRINT_STATE_PERCENT;
                }
                else
                {
                    vga_putchar(c);
                }
                break;

            case KPRINT_STATE_WIDTH:
                if (c >= '0' && c <= '9')
                {
                    unsigned int digit = (unsigned int)(c - '0');

                    if (width > (KPRINT_WIDTH_MAX - digit) / 10)
                        width = KPRINT_WIDTH_MAX;
                    else
                        width = width * 10 + digit;
                    break;
                }

                state = KPRINT_STATE_PERCENT;
                continue;

            case KPRINT_STATE_LENGTH:
                if (c == 'l' && length == KPRINT_LENGTH_LONG)
                {
                    length = KPRINT_LENGTH_LONG_LONG;
                    break;
                }

                if (c == 'l' || c == 'z')
                {
                    kprint_spec_literal(spec_start, fmt);
                    state = KPRINT_STATE_NORMAL;
                    break;
                }

                state = KPRINT_STATE_PERCENT;
                continue;

            case KPRINT_STATE_PERCENT:
                if (c == 'l' || c == 'z')
                {
                    length = (c == 'l') ? KPRINT_LENGTH_LONG : KPRINT_LENGTH_SIZE;
                    state = KPRINT_STATE_LENGTH;
                    break;
                }

                if (c == '0')
                {
                    pad = '0';
                    state = KPRINT_STATE_WIDTH;
                    break;
                }

                if (c >= '1' && c <= '9')
                {
                    width = (unsigned int)(c - '0');
                    state = KPRINT_STATE_WIDTH;
                    break;
                }

                switch (c)
                {
                    case '%':
                        vga_putchar('%');
                        break;

                    case 'c':
                        ch = (char)va_arg(ap, int);
                        while (width > 1)
                        {
                            vga_putchar(' ');
                            width--;
                        }
                        vga_putchar(ch);
                        break;

                    case 's':
                        s = va_arg(ap, const char *);
                        if (s == 0)
                            s = "(null)";

                        len = kstrlen(s);
                        while (width > len)
                        {
                            vga_putchar(' ');
                            width--;
                        }
                        vga_puts(s);
                        break;

                    case 'u':
                    case 'x':
                    case 'X':
                        switch (length)
                        {
                            case KPRINT_LENGTH_LONG_LONG:
                                value = va_arg(ap, unsigned long long);
                                break;

                            case KPRINT_LENGTH_LONG:
                                value = va_arg(ap, unsigned long);
                                break;

                            case KPRINT_LENGTH_SIZE:
                                value = va_arg(ap, size_t);
                                break;

                            default:
                                value = va_arg(ap, unsigned int);
                                break;
                        }

                        base = (c == 'u') ? 10 : 16;
                        kprint_number_format(value, base, (c == 'X'), false, width, pad);
                        break;

                    case 'd':
                    case 'i':
                        switch (length)
                        {
                            case KPRINT_LENGTH_LONG_LONG:
                                signed_value = va_arg(ap, long long);
                                break;

                            case KPRINT_LENGTH_LONG:
                                signed_value = va_arg(ap, long);
                                break;

                            case KPRINT_LENGTH_SIZE:
                                signed_value = va_arg(ap, ptrdiff_t);
                                break;

                            default:
                                signed_value = va_arg(ap, int);
                                break;
                        }

                        negative = signed_value < 0;
                        value = (uint64_t)signed_value;
                        if (negative)
                            value = 0 - value;

                        kprint_number_format(value, 10, false, negative, width, pad);
                        break;

                    /* Unlike printf, %p always uses the full pointer width and
                       ignores the format's own width and padding. */
                    case 'p':
                        value = (uint64_t)(uintptr_t)va_arg(ap, const void *);
                        vga_puts("0x");
                        kprint_number_format(value, 16, false, false, 2 * sizeof(void *), '0');
                        break;

                    default:
                        kprint_spec_literal(spec_start, fmt);
                        break;
                }

                state = KPRINT_STATE_NORMAL;
                break;
        }

        fmt++;
    }

    if (state != KPRINT_STATE_NORMAL)
        vga_puts(spec_start);
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

    kputs("hexdump len ");
    kprint_number_format(size, 10, false, false, 0, ' ');
    kputs("\n");

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
