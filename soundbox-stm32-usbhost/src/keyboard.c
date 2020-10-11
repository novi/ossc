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

uint8_t hidkeycodeToNextscancode(uint8_t scancode)
{
    return 0x55; // TODO:
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
            HAL_StatusTypeDef result = SendSPIData(data, 3);
            if (result != HAL_OK) {
                LOG("spi send error %d", result);
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
                data[2] = hidkeycodeToNextscancode(keyState[i].hidKeycode) | 0x80; // break code
                setKeyBreak(keyState[i].hidKeycode);
                normalKeySent = 1;
                HAL_StatusTypeDef result = SendSPIData(data, 3);
                if (result != HAL_OK) {
                    LOG("spi send error %d", result);
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
