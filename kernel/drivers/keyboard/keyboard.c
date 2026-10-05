#include "drivers/keyboard/keyboard.h"

#include "drivers/keyboard/ps2/ps2_keyboard.h"

static volatile keyboard_handler_t kbd_handler;

void keyboard_set_handler(keyboard_handler_t handler)
{
    kbd_handler = handler;
}

void keyboard_emit(char c)
{
    keyboard_handler_t handler = kbd_handler;

    if (handler != 0)
        handler(c);
}

void keyboard_init(void)
{
    kbd_handler = 0;

    ps2_keyboard_init();
}
