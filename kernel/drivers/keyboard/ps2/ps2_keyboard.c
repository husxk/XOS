#include "drivers/keyboard/ps2/ps2_keyboard.h"

#include "cpu/io.h"
#include "cpu/irq.h"
#include "cpu/isr.h"
#include "cpu/pic.h"
#include "drivers/keyboard/keyboard.h"

#define PS2_IRQ         1
#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64

/*
 * Status register bit 0 (Output Buffer Full): set while a byte read by
 * firmware or the keyboard is waiting in the data port, cleared the
 * moment that byte is read out of PS2_DATA_PORT.
 */
#define PS2_STATUS_OBF  0x01

/*
 * Bit 7 of a scan code marks a key release.
 *
 * The original XT keyboard had 83 keys, so make codes fit in seven
 * bits and the eighth was free to carry the press/release flag:
 *
 *      0x1E = 0001 1110    A pressed  (make)
 *      0x9E = 1001 1110    A released (break)
 *
 * One mask tells the two apart.
 */
#define PS2_BREAK_MASK      0x80
#define PS2_SCAN_LSHIFT     0x2A
#define PS2_SCAN_RSHIFT     0x36
#define PS2_SCAN_CAPSLOCK   0x3A

/*
 * Scan Code Set 1, US QWERTY: the character each key produces, indexed
 * by make code. The caller masks the break bit off the scan code before
 * indexing, so only 0x00-0x7F ever reach these arrays.
 *
 * An entry's position is its entire meaning and nothing may be skipped.
 * Zero means the key produces no character (modifiers, function keys)
 * and is dropped by the caller; entries past the last initialiser are
 * zero-filled by the compiler, which is exactly right for Caps Lock, the
 * function keys and the keypad.
 *
 * The two arrays are the unshifted and shifted faces of the same keys,
 * so they must stay aligned slot for slot: the caller picks between them
 * by Shift state and relies on identical indices meaning the same key.
 */
static const char ps2_map[128] =
{
    0,    0x1B, '1',  '2',  '3',  '4',  '5',  '6',   /* 0x00 */
    '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',  /* 0x08 */
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   /* 0x10 */
    'o',  'p',  '[',  ']',  '\n', 0,    'a',  's',   /* 0x18 */
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',   /* 0x20 */
    '\'', '`',  0,    '\\', 'z',  'x',  'c',  'v',   /* 0x28 */
    'b',  'n',  'm',  ',',  '.',  '/',  0,    '*',   /* 0x30 */
    0,    ' ',  0,                                   /* 0x38 */
};

static const char ps2_map_shift[128] =
{
    0,    0x1B, '!',  '@',  '#',  '$',  '%',  '^',   /* 0x00 */
    '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',  /* 0x08 */
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   /* 0x10 */
    'O',  'P',  '{',  '}',  '\n', 0,    'A',  'S',   /* 0x18 */
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   /* 0x20 */
    '"',  '~',  0,    '|',  'Z',  'X',  'C',  'V',   /* 0x28 */
    'B',  'N',  'M',  '<',  '>',  '?',  0,    '*',   /* 0x30 */
    0,    ' ',  0,                                   /* 0x38 */
};

static unsigned char shift_down;
static unsigned char caps_lock;

/* Swap the case of an ASCII letter; leave digits, symbols and 0 untouched. */
static char ps2_swap_case(char c)
{
    if (c >= 'a' && c <= 'z')
        return (char)(c - ('a' - 'A'));

    if (c >= 'A' && c <= 'Z')
        return (char)(c + ('a' - 'A'));

    return c;
}

static void ps2_keyboard_isr(interrupt_frame_t *frame)
{
    unsigned char scancode;
    unsigned char make;
    unsigned char code;
    char c;

    (void)frame;

    /*
     * The controller holds one byte and refuses further input until it
     * is collected, and the PIC blocks equal or lower priority
     * interrupts until it is acknowledged. Both happen first so that
     * no early return can skip them and wedge the keyboard.
     */
    scancode = io_inb(PS2_DATA_PORT);
    irq_ack(PS2_IRQ);

    /*
     * Split the press/release flag from the make code. Releases used to
     * be discarded outright, but Shift has to be followed up as well as
     * down, so we keep the make code and decide per key what to do.
     */
    make = (scancode & PS2_BREAK_MASK) == 0;
    code = (unsigned char)(scancode & ~PS2_BREAK_MASK);

    if (code == PS2_SCAN_LSHIFT || code == PS2_SCAN_RSHIFT)
    {
        shift_down = make;
        return;
    }

    if (code == PS2_SCAN_CAPSLOCK)
    {
        /* A lock toggles once per press; the matching release is ignored. */
        if (make)
            caps_lock = !caps_lock;

        return;
    }

    if (!make)
        return;

    c = (shift_down ? ps2_map_shift : ps2_map)[code];

    if (c == 0)
        return;

    /*
     * Caps Lock flips letter case on top of Shift, so Shift and Caps Lock
     * together land back on lowercase. It must not touch digits or
     * symbols, which ps2_swap_case leaves unchanged.
     */
    if (caps_lock)
        c = ps2_swap_case(c);

    keyboard_emit(c);
}

void ps2_keyboard_init(void)
{
    /*
     * Drain any byte the firmware left in the output buffer before
     * unmasking the IRQ.
     *
     * The controller holds exactly one byte and raises IRQ1 only on the
     * edge of a fresh byte arriving. A byte left over from POST (a self
     * test result, a command ACK, a key pressed during boot) keeps the
     * output buffer full: no new byte is accepted and no new edge is
     * produced, so the first real key press would never reach the ISR
     * and the keyboard looks dead. Reading PS2_DATA_PORT clears OBF; the
     * loop guards against more than one stale byte being queued.
     */
    while (io_inb(PS2_STATUS_PORT) & PS2_STATUS_OBF)
        (void)io_inb(PS2_DATA_PORT);

    isr_install_irq(PS2_IRQ, ps2_keyboard_isr);
    pic_unmask(PS2_IRQ);
}
