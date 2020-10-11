#include "keyboard.h"
#include "stm32f4xx.h"
#include "stm32f4_generic.h"

typedef enum {
    SPIHeader_Keyboard = 1,
    SPIHeader_Mouse = 2,
    SPIHeader_Mic = 3
} SPIHeader;

void KeyboardHandleMouseInfo(HID_MOUSE_Info_TypeDef* info)
{

}

HAL_StatusTypeDef SendSPIData(uint8_t* buf, size_t size)
{
    LOG("send spi %02x %02x %02x ", buf[0], buf[1], buf[2]);
    return HAL_OK;

    WriteGPIO(PIN_OUT_SPI_SS, 1); // enable, low active
    HAL_StatusTypeDef result = HAL_SPI_Transmit(&hspi1, buf, size, 100);
    WriteGPIO(PIN_OUT_SPI_SS, 0);
    return result;
}

uint8_t hidkeycodeToNextscancode(uint8_t keycode)
{
    // https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2
    const uint8_t keymap[] = {
        0, // 0
        0, // 1
        0, // 2
        0, // 3

        0x39, // KEY_A 0x04
        0x35,
        0x33,
        0x3b,
        0x44,
        0x3c,
        0x3d,
        0x40,
        0x06,
        0x3f,
        0x3e,
        0x2d,
        0x36,
        0x37,
        0x07,
        0x08, // KEY_P
        0x42,
        0x45,
        0x3a,
        0x48,
        0x46,
        0x34,
        0x43,
        0x32,
        0x47,
        0x31, // KEY_Z

        0x4a, // KEY_1,
        0x4b,
        0x4c,
        0x4d,
        0x50,
        0x4f,
        0x4e,
        0x1e,
        0x1f,
        0x20, // KEY_0

        0x2a, // KEY_ENTER
        0x49, // KEY_ESC
        0x1b, // KEY_BACKSPACE
        0x41, // KEY_TAB
        0x38, // KEY_SPACE
        0x1d, // KEY_MINUS
        0x1c, // KEY_EQUAL
        0x05, // KEY_LEFTBRACE
        0x04, // KEY_RIGHTBRACE
        0x03, // KEY_BACKSLASH
        0,// KEY_HASHTILDE // Keyboard Non-US # and ~
        0x2c, // KEY_SEMICOLON
        0x2b, // KEY_APOSTROPHE
        0x26, // KEY_GRAVE
        0x2e, // KEY_COMMA
        0x2f, // KEY_DOT
        0x30, // KEY_SLASH
        0, // KEY_CAPSLOCK, // TODO: map to control

        0x01, // KEY_F1, // Brightness down
        0x19, // F2, // Brightness up
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0x02, // F11, Audio lower
        0x1a, // KEY_F12, Audio higher

        0, // KEY_SYSRQ
        0, // KEY_SCROLLLOCK
        0, // KEY_PAUSE
        0, // KEY_INSERT
        0, // KEY_HOME
        0, // KEY_PAGEUP
        0, // KEY_DELETE
        0, // KEY_END
        0, // KEY_PAGEDOWN

        0x10, // KEY_RIGHT
        0x09, // KEY_LEFT
        0x0f, // KEY_DOWN
        0x16, // KEY_UP 

        0, // KEY_NUMLOCK
        0x28, // KEY_KPSLASH
        0x25, // KEY_KPASTERISK
        0x24, // KEY_KPMINUS
        0x15, // KEY_KPPLUS
        0x0d, // KEY_KPENTER
        0x11, // KEY_KP1
        0x17, // KEY_KP2
        0x14, // KEY_KP3
        0x12, // KEY_KP4
        0x18, // KEY_KP5
        0x13, // KEY_KP6
        0x21, // KEY_KP7
        0x22, // KEY_KP8
        0x23, // KEY_KP9
        0x0b, // KEY_KP0
        0x0c, // KEY_KPDOT
        
    };
    if (sizeof(keymap) > keycode) {
        uint8_t c = keymap[keycode];
        if (c) return c;
    }
    LOG("unknown hid keycode 0x%02x", keycode);
    return 0;
}

struct KeyState {
    uint8_t isValid;
    uint8_t hidKeycode;
};

#define KEY_ROLLOVER_COUNT (2) // NeXT keyboard is max 2 keys

static struct KeyState keyState[KEY_ROLLOVER_COUNT] = {{0},{0}};

void setKeyMake(uint8_t hidKeycode)
{
    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        if (!keyState[i].isValid) {
            keyState[i].isValid = 1;
            keyState[i].hidKeycode = hidKeycode;
            return;
        }
    }
    LOG("ERROR: can not store key make");
}

void setKeyBreak(uint8_t hidKeycode)
{
    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        if (keyState[i].isValid && keyState[i].hidKeycode == hidKeycode) {
            keyState[i].isValid = 0;
        } 
    }
}

uint8_t isKeyMake(uint8_t hidKeycode)
{
    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        if (keyState[i].isValid && keyState[i].hidKeycode == hidKeycode) {
            return 1;
        }
    }
    return 0;
}

void setAllKeyBreak()
{
    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        keyState[i].isValid = 0;
    }
}

void KeyboardHandleKeyboardInfo(HID_KEYBD_Info_TypeDef* info)
{
    uint8_t data[3] = {SPIHeader_Keyboard, 0, 0}; // Header, Modifier, Scancode

    uint8_t nextModifierCode = 0;
    if (info->lctrl || info->rctrl) {
        nextModifierCode |= 1 << 0;
    }
    if (info->lshift) {
        nextModifierCode |= 1 << 1;
    }
    if (info->rshift) {
        nextModifierCode |= 1 << 2;
    }
    if (info->lgui) {
        nextModifierCode |= 1 << 3;
    }
    if (info->rgui) {
        nextModifierCode |= 1 << 4;
    }
    if (info->lalt) {
        nextModifierCode |= 1 << 5;
    }
    if (info->ralt) {
        nextModifierCode |= 1 << 6;
    }

    uint8_t normalKeySent = 0;

    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        uint8_t hidkeycode = info->keys[i];
        if (hidkeycode && !isKeyMake(hidkeycode)) {
            // normal key make
            data[1] = nextModifierCode | 0x80;
            data[2] = hidkeycodeToNextscancode(hidkeycode);
            setKeyMake(hidkeycode);
            normalKeySent = 1;
            if (data[2]) { // not send unknown keycode
                HAL_StatusTypeDef result = SendSPIData(data, 3);
                if (result != HAL_OK) {
                    LOG("spi send error %d", result);
                }
            } else if (hidkeycode == 0x49) { // insert key
                LOG("TODO: handle insert key for power button.");
            } 
        }
    }
    

    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        if (keyState[i].isValid) {
            uint8_t isStoredKeyBreak = 1;
            for(uint8_t j = 0; j < KEY_ROLLOVER_COUNT; j++) {
                if (keyState[i].hidKeycode == info->keys[j]) {
                    isStoredKeyBreak = 0;
                }
            }
            if (isStoredKeyBreak) {
                // normal key break
                data[1] = nextModifierCode | 0x80;
                uint8_t nextscancode = hidkeycodeToNextscancode(keyState[i].hidKeycode);
                data[2] = nextscancode | 0x80; // break code
                setKeyBreak(keyState[i].hidKeycode);
                normalKeySent = 1;
                if (nextscancode) {
                    HAL_StatusTypeDef result = SendSPIData(data, 3);
                    if (result != HAL_OK) {
                        LOG("spi send error %d", result);
                    }
                }
            }
        }
    }
    
    uint8_t scanCodeForFirstKeyMake = 0;
    if (info->keys[0]) {
        scanCodeForFirstKeyMake = hidkeycodeToNextscancode(info->keys[0]);
    }

    if (!normalKeySent && nextModifierCode == 0) {
        // modifier break
        data[1] = 0x00;
        data[2] = scanCodeForFirstKeyMake ? scanCodeForFirstKeyMake : 0x80;
        HAL_StatusTypeDef result = SendSPIData(data, 3);
        if (result != HAL_OK) {
            LOG("spi send error %d", result);
        }
    } else if (!normalKeySent && nextModifierCode) {
        // only modifier make
        data[1] = nextModifierCode;
        data[2] = scanCodeForFirstKeyMake ? scanCodeForFirstKeyMake : 0x80;
        HAL_StatusTypeDef result = SendSPIData(data, 3);
        if (result != HAL_OK) {
            LOG("spi send error %d", result);
        }
    }
}
