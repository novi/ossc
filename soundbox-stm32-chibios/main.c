/*

   Copyright 2021-24 Yusuke Ito

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include "ch.h"
#include "hal.h"
#include "log.h"
#include "os/hal/extra/hal_i2c_extra.h"
#include "keyboard.h"
#include "mouse.h"
#include "../software/sys_controller/soundbox/mcu_i2c_structure.h"

#if HAL_USBH_USE_HID
#include "usbh/dev/hid.h"
#include "usbh/debug.h"		/* for _usbh_dbg/_usbh_dbgf */

#define SHOW_MOUSE_MOVE_LOG (0)
#define SHOW_KEYBOARD_MOUSE_CLICK_LOG (0)

static THD_WORKING_AREA(waTestHID, 1024);

static void _hid_report_callback(USBHHIDDriver *hidp, uint16_t len) {
    const uint8_t *report = (const uint8_t *)hidp->config->report_buffer;

    if (usbhhidGetType(hidp) == USBHHID_DEVTYPE_BOOT_MOUSE) {
        #if SHOW_MOUSE_MOVE_LOG
        _usbh_dbgf(hidp->dev->host, "Mouse report: buttons=%02x, Dx=%d, Dy=%d from device %x",
                report[0],
                (int8_t)report[1],
                (int8_t)report[2],
                hidp->dev);
        #endif
        #if SHOW_KEYBOARD_MOUSE_CLICK_LOG
        if (report[0] && report[1] == 0 && report[2] == 0) {
            _usbh_dbgf(hidp->dev->host, "Mouse report: buttons=%02x, Dx=%d, Dy=%d from device %x",
                report[0],
                (int8_t)report[1],
                (int8_t)report[2],
                hidp->dev);
        }
        #endif
        KeyboardHandleMouseInfo(report);
    } else if (usbhhidGetType(hidp) == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
        #if SHOW_KEYBOARD_MOUSE_CLICK_LOG
        if (report[0] || report[2] || report[3] || report[4] || report[5] || report[6] || report[7]) {
            _usbh_dbgf(hidp->dev->host, "Keyboard report: modifier=%02x, keys=%02x %02x %02x %02x %02x %02x from device %x",
                report[0],
                report[2],
                report[3],
                report[4],
                report[5],
                report[6],
                report[7],
                hidp->dev);
        }
        #endif
        KeyboardHandleKeyboardInfo(report);
    } else {
        _usbh_dbgf(hidp->dev->host, "Generic report, %d bytes", len);
    }
}

static USBH_DEFINE_BUFFER(uint8_t report[HAL_USBHHID_MAX_INSTANCES][8]);
static USBHHIDConfig hidcfg[HAL_USBHHID_MAX_INSTANCES];

static bool g_hasKeyboard = false; // usb keyboard connected

static void ThreadTestHID(void *p) {
    (void)p;
    uint8_t i;
    static uint8_t kbd_led_states[HAL_USBHHID_MAX_INSTANCES];

    chRegSetThreadName("HID");

    for (i = 0; i < HAL_USBHHID_MAX_INSTANCES; i++) {
        hidcfg[i].cb_report = _hid_report_callback;
        hidcfg[i].protocol = USBHHID_PROTOCOL_BOOT;
        hidcfg[i].report_buffer = report[i];
        hidcfg[i].report_len = 8;
    }

    for (;;) {
        bool hasKeyboard = false;
        for (i = 0; i < HAL_USBHHID_MAX_INSTANCES; i++) {
        	USBHHIDDriver *const hidp = &USBHHIDD[i];
            if (usbhhidGetState(hidp) == USBHHID_STATE_ACTIVE) {
                _usbh_dbgf(hidp->dev->host, "HID: Connected, HID%d", i);
                usbhhidStart(hidp, &hidcfg[i]);
                // if (usbhhidGetType(hidp) != USBHHID_DEVTYPE_GENERIC) {
                    usbhhidSetIdle(hidp, 0, 0);
                // }
                kbd_led_states[i] = 1;
            } else if (usbhhidGetState(hidp) == USBHHID_STATE_READY) {
                if (usbhhidGetType(hidp) == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
                    hasKeyboard = true;
                    USBH_DEFINE_BUFFER(uint8_t val);
                    val = kbd_led_states[i] << 1;
                    if (val == 0x08) {
                        val = 1;
                    }
                    val = 0; // turn off keyboard led status
                    usbhhidSetReport(hidp, 0, USBHHID_REPORTTYPE_OUTPUT, &val, 1);
                    kbd_led_states[i] = val;
                }
            }
        }
        g_hasKeyboard = hasKeyboard;        
        chThdSleepMilliseconds(200);
    }

}
#endif

/// --- i2c 
void onI2CSlaveRequest(I2CDriver *i2cp);
void onI2CSlaveReceive(I2CDriver *i2cp, const uint8_t *rxbuf, size_t rxbytes);
static size_t i2c_rx_bytes = 0;
static uint8_t i2c_has_slave_request = 0;
static uint8_t i2c_tx_buf[1] = {0};
static uint8_t i2c_rx_buf[2] = {0, 0};


static const I2CConfig i2c_config = {
    .op_mode = OPMODE_I2C,
    .clock_speed = 100000,
    // FAST_DUTY_CYCLE_2,
    .duty_cycle = STD_DUTY_CYCLE,
};

static void setup_i2c_(void)
{
    i2cStart(&I2CD2, &i2c_config);
    i2cSlaveOnRequest(&I2CD2, onI2CSlaveRequest);
    i2cSlaveMatchAddress(&I2CD2, 0xc8 >> 1); // I2C slave addr
    // i2cSlaveOnReceive(&I2CD2, onI2CSlaveReceive, i2c_rx_buf, 2);
    for (size_t i = 0; i < 100; i++) {
        if (i2cSlaveOnReceive(&I2CD2, onI2CSlaveReceive, i2c_rx_buf, 2) == MSG_OK) {
            break;
        }
    }
}

// -- SPI thread
static THD_WORKING_AREA(waThreadSPI, 1024);
static THD_FUNCTION(ThreadSPI, arg) {

  (void)arg;
  chRegSetThreadName("SPISend");
  while(true) {
    KeyboardSendPendingDataIfNeeded();
  }
}

// USB keyboard detect
static bool g_detect_nousb_keyboard = false;
static uint8_t g_detect_nousb_keyboard_count = 0;

// -- counter, LED blink thread
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  static uint8_t counter = 0;
  while (true) {
    palClearPad(GPIOB, GPIOB_STATUS_LED);
    osalThreadSleepMilliseconds(500);
    if (g_detect_nousb_keyboard && !g_hasKeyboard) {
        // no keyboard, blink LED
        palSetPad(GPIOB, GPIOB_STATUS_LED);
    }
    osalThreadSleepMilliseconds(500);
    // sdWrite(&SD2, (uint8_t*)"Hello\r\n", 7);
    LOG_MAIN("counter=%d, tick=%d\r\n", counter, osalOsGetSystemTimeX() );
    if (g_hasKeyboard) {
        g_detect_nousb_keyboard_count = 0;
    } else {
        // no usb keyboard
        if (g_detect_nousb_keyboard && g_detect_nousb_keyboard_count >= 20) { // 20 secs
            LOG_MAIN("no keyboard detected. restarting...\r\n");
            osalThreadSleepMilliseconds(500);
            NVIC_SystemReset(); // soft reset
        }
        if (g_detect_nousb_keyboard) {
            g_detect_nousb_keyboard_count++;
        }
    }

    counter++;
  }
}

void USBH_DEBUG_OUTPUT_CALLBACK(const uint8_t *buff, size_t len)
{
	sdWrite(&SD2, buff, len);
	sdWrite(&SD2, (const uint8_t *)"\r\n", 2);
}

void onI2CSlaveReceive(I2CDriver *i2cp, const uint8_t *rxbuf, size_t rxbytes)
{
    (void)i2cp;
	(void)rxbuf;
	(void)rxbytes;
    i2c_rx_bytes = rxbytes;
}

void onI2CSlaveRequest(I2CDriver *i2cp)
{
    (void)i2cp;
    i2c_tx_buf[0] = 0xa5; // TODO: dummy for now
    // TODO: send slave transmission if needed
    // i2cSlaveStartTransmission(&I2CD2, i2c_tx_buf, 1);
    i2c_has_slave_request = 1;
}

extern void spi_callback_data(SPIDriver *spip); // for keyboard.h
extern void spi_callback_error(SPIDriver *spip); 
// --- SPI
static const SPIConfig spi_config = {
    .circular = false,
    .data_cb = spi_callback_data, // callback
    .error_cb = spi_callback_error, // TODO: handle error
    .ssport = GPIOA,
    .sspad = GPIOA_SPI_SS,
    .cr1 = SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_CR1_SSM,
    .cr2 = 0
};

// --- GPIO
static uint8_t latest_ossc_power_state = 0;

static void update_mon_out_interface(void)
{
	latest_ossc_power_state = palReadPad(GPIOC, GPIOC_OSSC_POWER_SENS);
    if (latest_ossc_power_state) {
        palClearPad(GPIOC, GPIOC_MONOUT_INTERFACE_ENABLE); // enable
    } else {
        palSetPad(GPIOC, GPIOC_MONOUT_INTERFACE_ENABLE); // disable
    }
}

// returns non zero if fault
static uint8_t update_usb_host_power(void)
{
    palSetPad(GPIOC, GPIOC_USB_POWER);
    return 0;
    if (palReadPad(GPIOC, GPIOC_USB_FAULT)) {
        palClearPad(GPIOC, GPIOC_USB_POWER);
        return 1;
    } else {
        palSetPad(GPIOC, GPIOC_USB_POWER);
        return 0;
    }    
}

static void init_gpio_value(void)
{
	update_usb_host_power();
	update_mon_out_interface();
}

static bool shouldHandlePowerButton = false;
void HandlePowerButton(void) // from keyboard.h
{
    shouldHandlePowerButton = true;
}

static void process_i2c_recv_data(uint8_t data1, uint8_t data2)
{
    (void)data2;
    if ( (data1 & (1 << MCU_CONTROL_BIT_USE_SPEAKER)) ) {
        // set D-class amplifier On
        palSetPad(GPIOB, GPIOB_OUTPUT_AMPLIFIER_SHUTDOWN);
    } else {
        // off
        palClearPad(GPIOB, GPIOB_OUTPUT_AMPLIFIER_SHUTDOWN);
    }
    uint8_t mouseSpeed = (data1 >> MCU_CONTROL_BIT_MOUSE_SPEED) & 0x7; // 3bit
    MouseSetSpeed(mouseSpeed);

    g_detect_nousb_keyboard = (data1 & (1 << MCU_CONTROL_BIT_DETECT_NOUSB_KEYBOARD)) ? 1 : 0;
}

// void myOnSystemHalt(const char* reason)
// {
//     sdWrite(&SD2, reason, strlen(reason));
// }

static void printResetReason(uint32_t csr_reg)
{
    LOG_MAIN("reset detected: ");
    if (csr_reg & RCC_CSR_IWDGRSTF) {
        LOG_MAIN("IWDG ");
    } else if (csr_reg & RCC_CSR_WWDGRSTF) {
        LOG_MAIN("WWDG ");
    } else if (csr_reg & RCC_CSR_SFTRSTF) {
        LOG_MAIN("SFT ");
    } else if (csr_reg & RCC_CSR_PINRSTF) {
        LOG_MAIN("PIN ");
    } else if (csr_reg & RCC_CSR_PORRSTF) {
        LOG_MAIN("POR ");
    } else if (csr_reg & RCC_CSR_BORRSTF) {
        LOG_MAIN("BOR ");
    } else if (csr_reg & RCC_CSR_LPWRRSTF) {
        LOG_MAIN("LPWR ");
    }
    LOG_MAIN("\r\n");
}

int main(void)
{

    uint32_t csr_reg = RCC->CSR;
    //   IWDG->KR = 0x5555;
    //   IWDG->PR = 7;

    halInit();
    chSysInit();

    init_gpio_value();

    KeyboardInit();

    spiStart(&SPID1, &spi_config);
    spiUnselect(&SPID1);

    // Serial USART2
    // baud=115200
    sdStart(&SD2, NULL);
    LOG_MAIN("mcu started, waiting ossc starts\r\n");

    printResetReason(csr_reg);
    // clear reset reason flags
    RCC->CSR |= RCC_CSR_RMVF;

    osalThreadSleepMilliseconds(100);
    osalThreadSleepMilliseconds(1000);

    // osalDbgAssert(0, "test error");

    setup_i2c_();

    #if HAL_USBH_USE_HID
    chThdCreateStatic(waTestHID, sizeof(waTestHID), NORMALPRIO, ThreadTestHID, 0);
    #endif

    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);
    chThdCreateStatic(waThreadSPI, sizeof(waThreadSPI), NORMALPRIO, ThreadSPI, NULL);

    //start
#if STM32_USBH_USE_OTG1
    usbhStart(&USBHD1);
    _usbh_dbgf(&USBHD1, "USBH Started");
#endif

    for(;;) {

        update_mon_out_interface();
        if (update_usb_host_power()) {
            while (1) {
                LOG_MAIN("USB host power fault\r\n");
            }
        }

        // sdWrite(&SD2, (const uint8_t *)"loop\r\n", 6);

        // LOG("loop ossc sens = %d, usb fault = %d\r\n", latest_ossc_power_state, palReadPad(GPIOC, GPIOC_USB_FAULT) );

#if STM32_USBH_USE_OTG1
        usbhMainLoop(&USBHD1);
#endif
        osalThreadSleepMilliseconds(100);

        // IWDG->KR = 0xAAAA;

        if (i2c_rx_bytes) {
            LOG_DEBUG("i2c recv %d bytes, data = 0x%02x, 0x%02x", i2c_rx_bytes, i2c_rx_buf[0], i2c_rx_buf[1]);
            process_i2c_recv_data(i2c_rx_buf[0], i2c_rx_buf[1]);
            i2c_rx_bytes = 0;
        }

        if (I2CD2.errors) {
            LOG_MAIN("i2c error 0x%02x\r\n", I2CD2.errors);
            // osalThreadSleepMilliseconds(100);
            i2cStop(&I2CD2);
            setup_i2c_();
        }

        if (i2c_has_slave_request) {
            LOG_DEBUG("i2c has got slave request");
            // osalThreadSleepMilliseconds(100);
            i2cStop(&I2CD2);
            setup_i2c_();
            i2c_has_slave_request = 0;
        }

        if (shouldHandlePowerButton) {
            shouldHandlePowerButton = false;
            LOG_DEBUG("Handle Power Button\r\n");
            palSetPad(GPIOC, GPIOC_NEXT_POWERSW);
            osalThreadSleepMilliseconds(100);
            palClearPad(GPIOC, GPIOC_NEXT_POWERSW);
        }
    }
}
