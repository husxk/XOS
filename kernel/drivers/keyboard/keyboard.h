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

void keyboard_init(void);

/*
 * Called by a backend driver once it has decoded a character.
 *
 * Runs in interrupt context: it only enqueues the character into the
 * input buffer and returns, so the backend never blocks on a consumer.
 * On a full buffer the character is dropped. This is the backend-facing
 * half of the interface, not something the rest of the kernel calls.
 */
void keyboard_emit(char c);

/*
 * Returns the next buffered character, or -1 when none is available.
 * Drained outside interrupt context, typically by a consumer thread.
 */
int keyboard_getchar(void);
