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
#include "chprintf.h"
#include "usbh/debug.h"		/* for _usbh_dbg/_usbh_dbgf */
#if HAL_USBH_USE_HID
#include "usbh/dev/hid.h"
#include "chprintf.h"

static THD_WORKING_AREA(waTestHID, 1024);

static void _hid_report_callback(USBHHIDDriver *hidp, uint16_t len) {
    uint8_t *report = (uint8_t *)hidp->config->report_buffer;

    if (hidp->type == USBHHID_DEVTYPE_BOOT_MOUSE) {
        _usbh_dbgf(hidp->dev->host, "Mouse report: buttons=%02x, Dx=%d, Dy=%d from device %x",
                report[0],
                (int8_t)report[1],
                (int8_t)report[2],
                hidp->dev);
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

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  static uint8_t counter = 0;
  while (true) {
    palClearPad(GPIOC, GPIOC_LED_BLUE);
    osalThreadSleepMilliseconds(500);
    palSetPad(GPIOC, GPIOC_LED_BLUE);
    osalThreadSleepMilliseconds(500);
    // sdWrite(&SD2, (uint8_t*)"Hello\r\n", 7);
    chprintf((BaseSequentialStream*)&SD2, "hello %d, tick=%d\r\n", counter, osalOsGetSystemTimeX() );
    counter++;
  }
}

#if USBH_DEBUG_MULTI_HOST
void USBH_DEBUG_OUTPUT_CALLBACK(USBHDriver *host, const uint8_t *buff, size_t len) {
	(void)host;
  #error TODO
#else
void USBH_DEBUG_OUTPUT_CALLBACK(const uint8_t *buff, size_t len) {
#endif
	sdWrite(&SD2, buff, len);
	sdWrite(&SD2, (const uint8_t *)"\r\n", 2);
}

/*
 * Application entry point.
 */
int main(void) {

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

  /*
   * Activates the serial driver 2 using the driver default configuration.
   */
  sdStart(&SD2, NULL);
  sdWrite(&SD2, (const uint8_t *)"start\r\n", 7);
  osalThreadSleepMilliseconds(100);

  #if STM32_USBH_USE_OTG1
    //VBUS - configured in board.h
    //USB_FS - configured in board.h
  #endif

  #if STM32_USBH_USE_OTG2
  #error "TODO: Initialize USB_HS pads"
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
    _usbh_dbgf(&USBHD1, "Started");
#endif
#if STM32_USBH_USE_OTG2
    usbhStart(&USBHD2);
    _usbh_dbgf(&USBHD2, "Started");
#endif

    for(;;) {
        // sdWrite(&SD2, (const uint8_t *)"loop\r\n", 6);
#if STM32_USBH_USE_OTG1
        usbhMainLoop(&USBHD1);
#endif
#if STM32_USBH_USE_OTG2
        usbhMainLoop(&USBHD2);
#endif
        osalThreadSleepMilliseconds(100);

        // IWDG->KR = 0xAAAA;
        
    }
}
