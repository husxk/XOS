#pragma once

/*
 * Hardware-independent keyboard interface.
 *
 * Nothing above this header knows how the keys arrived. A backend
 * driver (currently PS/2) decodes whatever its hardware produces and
 * hands over plain characters; swapping in a USB backend later means
 * writing a second backend that calls keyboard_emit(), with no change
 * anywhere else in the kernel.
 */

/* Consumer of decoded characters. */
typedef void (*keyboard_handler_t)(char c);

void keyboard_init(void);

/*
 * Installs the consumer. Until one is installed, decoded characters
 * are dropped. Should be called after keyboard_init().
 */
void keyboard_set_handler(keyboard_handler_t handler);

/*
 * Called by a backend driver once it has decoded a character.
 *
 * Runs in interrupt context, so the installed handler must be short.
 * This is the backend-facing half of the interface, not something the
 * rest of the kernel should call.
 */
void keyboard_emit(char c);
