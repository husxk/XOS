#include "drivers/vga/vga.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_TAB     8
#define VGA_MEMORY  ((volatile unsigned short *)0xB8000)

static unsigned char vga_fg;
static unsigned char vga_bg;
static unsigned int vga_row;
static unsigned int vga_col;

static unsigned short vga_entry(char c)
{
    unsigned char color = (unsigned char)((vga_bg << 4) | (vga_fg & 0x0f));
    return (unsigned short)(unsigned char)c | ((unsigned short)color << 8);
}

static void vga_scroll(void)
{
    for (unsigned int y = 0; y < VGA_HEIGHT - 1; y++)
    {
        for (unsigned int x = 0; x < VGA_WIDTH; x++)
        {
            volatile unsigned short *dst = VGA_MEMORY + y * VGA_WIDTH + x;
            volatile unsigned short *src = VGA_MEMORY + (y + 1) * VGA_WIDTH + x;

            *dst = *src;
        }
    }

    for (unsigned int x = 0; x < VGA_WIDTH; x++)
    {
        volatile unsigned short *cell = VGA_MEMORY + (VGA_HEIGHT - 1) * VGA_WIDTH + x;

        *cell = vga_entry(' ');
    }

    vga_row = VGA_HEIGHT - 1;
}

static void vga_newline(void)
{
    vga_col = 0;
    vga_row++;

    if (vga_row >= VGA_HEIGHT)
        vga_scroll();
}

void vga_init(void)
{
    vga_fg = VGA_COLOR_WHITE;
    vga_bg = VGA_COLOR_BLACK;

    vga_row = 0;
    vga_col = 0;

    vga_clear();
}

void vga_clear(void)
{
    unsigned short blank = vga_entry(' ');

    for (unsigned int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
    {
        volatile unsigned short *cell = VGA_MEMORY + i;

        *cell = blank;
    }

    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(unsigned char fg, unsigned char bg)
{
    if (fg > VGA_COLOR_WHITE || bg > VGA_COLOR_WHITE)
        return;

    vga_fg = fg;
    vga_bg = bg;
}

void vga_putchar(char c)
{
    if (c == '\n')
    {
        vga_newline();
        return;
    }

    if (c == '\r')
    {
        vga_col = 0;
        return;
    }

    if (c == '\t')
    {
        vga_col += VGA_TAB - (vga_col % VGA_TAB);

        if (vga_col >= VGA_WIDTH)
            vga_newline();

        return;
    }

    if (vga_col >= VGA_WIDTH)
        vga_newline();

    volatile unsigned short *cell = VGA_MEMORY + vga_row * VGA_WIDTH + vga_col;

    *cell = vga_entry(c);
    vga_col++;

    if (vga_col >= VGA_WIDTH)
        vga_newline();
}

void vga_puts(const char *str)
{
    for (; *str; str++)
        vga_putchar(*str);
}
