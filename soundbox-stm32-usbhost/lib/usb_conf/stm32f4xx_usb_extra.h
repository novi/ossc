#ifndef _STM32F4xx_USB_EXTRA_H
#define _STM32F4xx_USB_EXTRA_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef USB_StopHostChannel(USB_OTG_GlobalTypeDef *USBx, uint8_t chnum);
HAL_StatusTypeDef HAL_HCD_StopHC(HCD_HandleTypeDef *hhcd, uint8_t chnum);

#endif // #ifndef _STM32F4xx_USB_EXTRA_H
