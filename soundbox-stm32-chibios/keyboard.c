#include "keyboard.h"
#include "log.h"
#include <string.h>

typedef enum {
    SPIHeader_Keyboard = 1,
    SPIHeader_Mouse = 2,
    SPIHeader_Mic = 3
} SPIHeader;

#define SPI_DATA_SIZE_MAX (3)
static uint8_t spi_send_buf[SPI_DATA_SIZE_MAX];
static uint8_t spi_recv_buf[SPI_DATA_SIZE_MAX];
volatile uint8_t spi_is_sending = 0;

void SendSPIData(uint8_t* buf, size_t size)
{
    osalDbgAssert(SPI_DATA_SIZE_MAX <= size, "SendSPIData max size exceed.");

    // SPI to FPGA
    // command, data[0], data[1]...
    LOG_DEBUG("send spi %02x %02x %02x", buf[0], buf[1], buf[2]);

    systime_t start, end;
    start = osalOsGetSystemTimeX();
    end = start + OSAL_MS2I(10); // in ms
    while (spi_is_sending) {
        if (!osalTimeIsInRangeX(osalOsGetSystemTimeX(), start, end)) {
            // timeout
            LOG_DEBUG("spi send data wait timeout");
            break;
        }
    }

    // for(size_t i = 0; spi_is_sending && i < 10000; i++);
    // while (spi_is_sending);    
    
    memcpy(spi_send_buf, buf, size);

    spi_is_sending = 1;
    spiSelectI(&SPID1);
    spiStartExchangeI(&SPID1, size, spi_send_buf, spi_recv_buf);
}

void spi_callback(SPIDriver *spip)
{
    (void)spip;
    spiUnselectI(&SPID1);
    spi_is_sending = 0;

    LOG_DEBUG("spi send callback state %d", spip->state);
}

static uint8_t mouse_left_up = 1;
static uint8_t mouse_right_up = 1;

//#define MOUSE_MOVE_SCALE_FACTOR 8

static int8_t mouseAccTable[] = {
    0, // 0
    1, // 1
    1, // 2
    1, // 3
    2, // 4
    2, // 5
    2, // 6
    3, // 7
    3, // 8
    4, // 9
    4, // 10
    5, // 11
    5, // 12
    6, // 13
    6, // 14
    7, // 15
};

void KeyboardHandleMouseInfo(const uint8_t *report)
{
    uint8_t is_report_left_down = report[0] & 0x1;
    uint8_t is_report_right_down = report[0] & 0x2;

    if (is_report_left_down && mouse_left_up) {
        mouse_left_up = 0; // mouse down
    } else if (!is_report_left_down && !mouse_left_up) {
        mouse_left_up = 1; // mouse up
    }
    if (is_report_right_down && mouse_right_up) {
        mouse_right_up = 0; // mouse down
    } else if (!is_report_right_down && !mouse_right_up) {
        mouse_right_up = 1; // mouse up
    }

    uint8_t d0 = mouse_left_up;
    uint8_t d1 = mouse_right_up;

    int8_t movX = report[1];
    int8_t movY = report[2];
    if (movX != 0 || movY != 0) {

        uint8_t absx = movX < 0 ? -movX : movX;
        int8_t valx;
        if (sizeof(mouseAccTable) > absx) {
            valx = movX < 0 ? mouseAccTable[absx] : -mouseAccTable[absx]; // swap sign
        } else {
            // valx = -(movX/MOUSE_MOVE_SCALE_FACTOR);
            valx = movX < 0 ? mouseAccTable[sizeof(mouseAccTable)-1] : -mouseAccTable[sizeof(mouseAccTable)-1];
        }
        d0 |= (valx << 1 ) & 0xfe;


        uint8_t absy = movY < 0 ? -movY : movY;
        int8_t valy;
        if (sizeof(mouseAccTable) > absy) {
            valy = movY < 0 ? mouseAccTable[absy] : -mouseAccTable[absy]; // swap sign
        } else {
            //valy = -(movY/MOUSE_MOVE_SCALE_FACTOR);
            valy = movY < 0 ? mouseAccTable[sizeof(mouseAccTable)-1] : -mouseAccTable[sizeof(mouseAccTable)-1];
        }
        d1 |= (valy << 1 ) & 0xfe;
        LOG_DEBUG("raw (%d, %d), mov (%d, %d)", report[1], report[2], movX, movY);
    }

    uint8_t data[3] = {SPIHeader_Mouse, 0, 0}; // Header
    data[1] = d0;
    data[2] = d1;
    SendSPIData(data, 3);
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
        0, // KEY_CAPSLOCK,

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
    LOG("unknown hid keycode 0x%02x that there is not on next keyboard", keycode);
    return 0;
}

struct KeyState {
    uint8_t isValid;
    uint8_t hidKeycode;
};

#define KEY_ROLLOVER_COUNT (2) // NeXT keyboard is max 2 keys

static struct KeyState keyState[6] = {{0},{0},{0},{0},{0},{0}};

void setKeyMake(uint8_t hidKeycode)
{
    for(uint8_t i = 0; i < 6; i++) {
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
    for(uint8_t i = 0; i < 6; i++) {
        if (keyState[i].isValid && keyState[i].hidKeycode == hidKeycode) {
            keyState[i].isValid = 0;
        } 
    }
}

uint8_t isKeyMake(uint8_t hidKeycode)
{
    for(uint8_t i = 0; i < 6; i++) {
        if (keyState[i].isValid && keyState[i].hidKeycode == hidKeycode) {
            return 1;
        }
    }
    return 0;
}

void setAllKeyBreak(void)
{
    for(uint8_t i = 0; i < 6; i++) {
        keyState[i].isValid = 0;
    }
}

static uint8_t latestModifier = 0;

#define USE_RIGHT_CONTROL_AS_COMMAND 1

#define REPORT_MODIFIER_CTRL_LEFT 0
#define REPORT_MODIFIER_SHIFT_LEFT 1
#define REPORT_MODIFIER_ALT_LEFT 2
#define REPORT_MODIFIER_GUI_LEFT 3
#define REPORT_MODIFIER_CTRL_RIGHT 4
#define REPORT_MODIFIER_SHIFT_RIGHT 5
#define REPORT_MODIFIER_ALT_RIGHT 6
#define REPORT_MODIFIER_GUI_RIGHT 7

void KeyboardHandleKeyboardInfo(const uint8_t *report)
{
    // data to send
    uint8_t data[3] = {SPIHeader_Keyboard, 0, 0}; // Header, Modifier, Scancode
    const uint8_t *report_keys = &report[2];

    uint8_t nextModifierCode = 0;

    // Map caps lock to control key
    for(uint8_t i = 0; i < 6; i++) {
        if (report_keys[i] == 0x39) { // USB(HID) caps lock key
            nextModifierCode |= 1 << 0; // control key
        }
    }

    #if USE_RIGHT_CONTROL_AS_COMMAND
    if ( (report[0] & (1 << REPORT_MODIFIER_CTRL_LEFT)) ) {
    #else
    if ( (report[0] & (1 << REPORT_MODIFIER_CTRL_LEFT)) || (report[0] & (1 << REPORT_MODIFIER_CTRL_RIGHT)) ) {
    #endif
        nextModifierCode |= 1 << 0;
    }
    if ( (report[0] & (1 << REPORT_MODIFIER_SHIFT_LEFT)) ) {
        nextModifierCode |= 1 << 1;
    }
    if ( (report[0] & (1 << REPORT_MODIFIER_SHIFT_RIGHT)) ) {
        nextModifierCode |= 1 << 2;
    }
    if ( (report[0] & (1 << REPORT_MODIFIER_GUI_LEFT)) ) {
        nextModifierCode |= 1 << 3;
    }
    #if USE_RIGHT_CONTROL_AS_COMMAND
    if ((report[0] & (1 << REPORT_MODIFIER_GUI_RIGHT)) || (report[0] & (1 << REPORT_MODIFIER_CTRL_RIGHT)) ) {
    #else
    if ( (report[0] & (1 << REPORT_MODIFIER_GUI_RIGHT)) ) {
    #endif
        nextModifierCode |= 1 << 4;
    }
    if ( (report[0] & (1 << REPORT_MODIFIER_ALT_LEFT)) ) {
        nextModifierCode |= 1 << 5;
    }
    if ( (report[0] & (1 << REPORT_MODIFIER_ALT_RIGHT)) ) {
        nextModifierCode |= 1 << 6;
    }

    uint8_t normalKeySent = 0;

    for(uint8_t i = 0; i < KEY_ROLLOVER_COUNT; i++) {
        uint8_t hidkeycode = report_keys[i];
        if (hidkeycode && !isKeyMake(hidkeycode)) {
            // normal key make
            data[2] = nextModifierCode | 0x80;
            data[1] = hidkeycodeToNextscancode(hidkeycode);
            setKeyMake(hidkeycode);
            normalKeySent = 1;
            if (data[1]) { // not send unknown keycode
                SendSPIData(data, 3);
            } else if (hidkeycode == 0x49 || // insert key
                        hidkeycode == 0x3d) { // F4 key
                HandlePowerButton();
            } 
        }
    }
    

    for(uint8_t i = 0; i < 6; i++) {
        if (keyState[i].isValid) {
            uint8_t isStoredKeyBreak = 1;
            for(uint8_t j = 0; j < KEY_ROLLOVER_COUNT; j++) {
                if (keyState[i].hidKeycode == report_keys[j]) {
                    isStoredKeyBreak = 0;
                    break;
                }
            }
            if (isStoredKeyBreak) {
                // normal key break
                data[2] = nextModifierCode | 0x80;
                uint8_t nextscancode = hidkeycodeToNextscancode(keyState[i].hidKeycode);
                data[1] = nextscancode | 0x80; // break code
                setKeyBreak(keyState[i].hidKeycode);
                normalKeySent = 1;
                if (nextscancode) {
                    SendSPIData(data, 3);
                }
            }
        }
    }
    
    uint8_t scanCodeForFirstKeyMake = 0;
    if (report_keys[0]) {
        scanCodeForFirstKeyMake = hidkeycodeToNextscancode(report_keys[0]);
    }

    if (!normalKeySent && latestModifier != nextModifierCode) {
        // modifier changed
        if (nextModifierCode == 0) {
            // modifier break
            data[2] = 0x00;
            data[1] = scanCodeForFirstKeyMake ? scanCodeForFirstKeyMake : 0x80;
            SendSPIData(data, 3);
        } else {
            // only modifier make
            data[2] = nextModifierCode;
            data[1] = scanCodeForFirstKeyMake ? scanCodeForFirstKeyMake : 0x80;
            SendSPIData(data, 3);
        }
    }

    

    latestModifier = nextModifierCode;
}
