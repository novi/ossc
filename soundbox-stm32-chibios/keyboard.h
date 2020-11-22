#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include <inttypes.h>

extern void HandlePowerButton(void);

void KeyboardHandleMouseInfo(const uint8_t *report);
void KeyboardHandleKeyboardInfo(const uint8_t *report);

#endif // _KEYBOARD_H