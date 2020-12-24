#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "usbh_hid.h"

extern void HandlePowerButton();

void KeyboardHandleMouseInfo(HID_MOUSE_Info_TypeDef* info);
void KeyboardHandleKeyboardInfo(HID_KEYBD_Info_TypeDef* info);

#endif // _KEYBOARD_H