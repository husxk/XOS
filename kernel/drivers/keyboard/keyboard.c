#include "drivers/keyboard/keyboard.h"

#include "drivers/keyboard/ps2/ps2_keyboard.h"
#include "util/ring_buffer.h"

#define KBD_RING_CAP 128u

static unsigned char kbd_storage[KBD_RING_CAP];
static ring_buffer_t kbd_ring;

void keyboard_emit(char c)
{
    ring_buffer_put(&kbd_ring, &c);
}

int keyboard_getchar(void)
{
    char c;

    if (ring_buffer_get(&kbd_ring, &c))
        return (int)(unsigned char)c;

    return -1;
}

void keyboard_init(void)
{
    ring_buffer_init(&kbd_ring, kbd_storage, KBD_RING_CAP, sizeof(char));

    ps2_keyboard_init();
}
