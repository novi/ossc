#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "usbh/dev/hid.h"

extern void HandlePowerButton(void);

void KeyboardHandleMouseInfo(USBHHIDDriver *hidp);
void KeyboardHandleKeyboardInfo(USBHHIDDriver *hidp);

#endif // _KEYBOARD_H