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
#define PS2_BREAK_MASK  0x80

/*
 * Scan Code Set 1, unshifted, US QWERTY.
 *
 * Index is the make code, so an entry's position is its entire meaning
 * and nothing may be skipped. Zero means the key produces no character
 * (modifiers, function keys) and is dropped by the caller. Break codes
 * are filtered out before the lookup, so only 0x00-0x7F ever reach this
 * array; entries past the last initialiser are zero-filled by the compiler,
 * which is exactly right for CapsLock, the function keys and the keypad.
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

static void ps2_keyboard_isr(interrupt_frame_t *frame)
{
    unsigned char scancode;
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

    if (scancode & PS2_BREAK_MASK)
        return;

    c = ps2_map[scancode];

    if (c != 0)
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
