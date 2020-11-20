/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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
// #include "rt_test_root.h"
// #include "oslib_test_root.h"
#include "log.h"
#include "os/hal/extra/hal_i2c_extra.h"
#include "usbh/debug.h"		/* for _usbh_dbg/_usbh_dbgf */
#if HAL_USBH_USE_HID
#include "usbh/dev/hid.h"
#include "chprintf.h"
#include "keyboard.h"

static THD_WORKING_AREA(waTestHID, 1024);

static void _hid_report_callback(USBHHIDDriver *hidp, uint16_t len) {
    uint8_t *report = (uint8_t *)hidp->config->report_buffer;

    if (hidp->type == USBHHID_DEVTYPE_BOOT_MOUSE) {
        _usbh_dbgf(hidp->dev->host, "Mouse report: buttons=%02x, Dx=%d, Dy=%d from device %x",
                report[0],
                (int8_t)report[1],
                (int8_t)report[2],
                hidp->dev);
        KeyboardHandleMouseInfo(hidp);
    } else if (hidp->type == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
        _usbh_dbgf(hidp->dev->host, "Keyboard report: modifier=%02x, keys=%02x %02x %02x %02x %02x %02x from device %x",
                report[0],
                report[2],
                report[3],
                report[4],
                report[5],
                report[6],
                report[7],
                hidp->dev);
        KeyboardHandleKeyboardInfo(hidp);
    } else {
        _usbh_dbgf(hidp->dev->host, "Generic report, %d bytes", len);
    }
}

static USBH_DEFINE_BUFFER(uint8_t report[HAL_USBHHID_MAX_INSTANCES][8]);
static USBHHIDConfig hidcfg[HAL_USBHHID_MAX_INSTANCES];

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
        for (i = 0; i < HAL_USBHHID_MAX_INSTANCES; i++) {
        	USBHHIDDriver *const hidp = &USBHHIDD[i];
            if (usbhhidGetState(hidp) == USBHHID_STATE_ACTIVE) {
                _usbh_dbgf(hidp->dev->host, "HID: Connected, HID%d", i);
                usbhhidStart(hidp, &hidcfg[i]);
                if (usbhhidGetType(hidp) != USBHHID_DEVTYPE_GENERIC) {
                    usbhhidSetIdle(hidp, 0, 0);
                }
                kbd_led_states[i] = 1;
            } else if (usbhhidGetState(hidp) == USBHHID_STATE_READY) {
                if (usbhhidGetType(hidp) == USBHHID_DEVTYPE_BOOT_KEYBOARD) {
                    USBH_DEFINE_BUFFER(uint8_t val);
                    val = kbd_led_states[i] << 1;
                    if (val == 0x08) {
                        val = 1;
                    }
                    usbhhidSetReport(hidp, 0, USBHHID_REPORTTYPE_OUTPUT, &val, 1);
                    kbd_led_states[i] = val;
                }
            }
        }
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
    OPMODE_I2C,
    100000,
    // FAST_DUTY_CYCLE_2,
    STD_DUTY_CYCLE
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

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  static uint8_t counter = 0;
  while (true) {
    palClearPad(GPIOB, GPIOB_STATUS_LED);
    osalThreadSleepMilliseconds(500);
    palSetPad(GPIOB, GPIOB_STATUS_LED);
    osalThreadSleepMilliseconds(500);
    // sdWrite(&SD2, (uint8_t*)"Hello\r\n", 7);
    LOG("hello %d, tick=%d\r\n", counter, osalOsGetSystemTimeX() );
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
    i2c_tx_buf[0] = 0xa5;
    i2cSlaveStartTransmission(&I2CD2, i2c_tx_buf, 1);
    i2c_has_slave_request = 1;
}

extern void spi_callback(SPIDriver *spip);
// --- SPI
static const SPIConfig spi_config = {
    false, // no circular buffer
    spi_callback, // callback
    GPIOA,
    GPIOA_SPI_SS,
    SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_CR1_SSM,
    0
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

static void update_usb_host_power(void)
{
    palSetPad(GPIOC, GPIOC_USB_POWER);
}

static void init_gpio_value(void)
{
	palClearPad(GPIOC, GPIOC_NEXT_POWERSW);
	palClearPad(GPIOB, GPIOB_STATUS_LED);
	update_usb_host_power();
	update_mon_out_interface();
}

void HandlePowerButton(void)
{
    palSetPad(GPIOC, GPIOC_NEXT_POWERSW);
    osalThreadSleepMilliseconds(100);
    palClearPad(GPIOC, GPIOC_NEXT_POWERSW);
}

// void myOnSystemHalt(const char* reason)
// {
//     sdWrite(&SD2, reason, strlen(reason));
// }

/*
 * Application entry point.
 */
int main(void)
{

    //   IWDG->KR = 0x5555;
    //   IWDG->PR = 7;

    /*
    * System initializations.
    * - HAL initialization, this also initializes the configured device drivers
    *   and performs the board-specific initializations.
    * - Kernel initialization, the main() function becomes a thread and the
    *   RTOS is active.
    */
    halInit();
    chSysInit();

    init_gpio_value();

    spiStart(&SPID1, &spi_config);
    spiUnselect(&SPID1);

    /*
    * Activates the serial driver 2 using the driver default configuration.
    */
    sdStart(&SD2, NULL);
    LOG("mcu started, waiting ossc starts\r\n");

    osalThreadSleepMilliseconds(100);

    // osalDbgAssert(0, "test error");

    setup_i2c_();

    #if STM32_USBH_USE_OTG1
    //VBUS - configured in board.h
    //USB_FS - configured in board.h
    #endif

    #if HAL_USBH_USE_HID
    chThdCreateStatic(waTestHID, sizeof(waTestHID), NORMALPRIO, ThreadTestHID, 0);
    #endif

    /*
    * Creates the blinker thread.
    */
    chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

    //start
#if STM32_USBH_USE_OTG1
    usbhStart(&USBHD1);
    _usbh_dbgf(&USBHD1, "USBH Started");
#endif

    for(;;) {

        update_mon_out_interface();

        // sdWrite(&SD2, (const uint8_t *)"loop\r\n", 6);

        LOG("loop ossc sens = %d, usb fault = %d\r\n", latest_ossc_power_state, palReadPad(GPIOC, GPIOC_USB_FAULT) );

#if STM32_USBH_USE_OTG1
        usbhMainLoop(&USBHD1);
#endif
        osalThreadSleepMilliseconds(100);

        // IWDG->KR = 0xAAAA;

        if (i2c_rx_bytes) {
            LOG("i2c recv %d bytes, data = %d, %d\r\n", i2c_rx_bytes, i2c_rx_buf[0], i2c_rx_buf[1]);
            i2c_rx_bytes = 0;
        }

        if (I2CD2.errors) {
            LOG("i2c error 0x%02x\r\n", I2CD2.errors);
            // osalThreadSleepMilliseconds(100);
            i2cStop(&I2CD2);
            setup_i2c_();
        }

        if (i2c_has_slave_request) {
            LOG("i2c has got slave request\r\n");
            // osalThreadSleepMilliseconds(100);
            i2cStop(&I2CD2);
            setup_i2c_();
            i2c_has_slave_request = 0;
        }
        
    }
}
