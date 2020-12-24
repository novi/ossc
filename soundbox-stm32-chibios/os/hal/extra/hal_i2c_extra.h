/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

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
/*
   Concepts and parts of this file have been contributed by Uladzimir Pylinsky
   aka barthess.
 */

/**
 * @file    hal_i2c.h
 * @brief   I2C Driver macros and structures.
 *
 * @addtogroup I2C
 * @{
 */

#ifndef HAL_I2C_EXTRA_H
#define HAL_I2C_EXTRA_H

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

#include "hal_i2c_lld.h"

#ifdef __cplusplus
extern "C" {
#endif

#if STM32_I2C_SLAVE_ENABLE
  msg_t i2cSlaveMatchAddress(I2CDriver *i2cp, i2caddr_t  i2cadr);
  msg_t i2cSlaveOnReceive(I2CDriver *i2cp, i2c_slave_receive_callback_t callback,
		                  uint8_t *rxbuf, size_t rxbytes);
  void i2cSlaveOnRequest(I2CDriver *i2cp, i2c_slave_transmit_callback_t callback);
  msg_t i2cSlaveStartTransmission(I2CDriver *i2cp, const uint8_t *txbuf,
		  		                  size_t txbytes);
#endif /* STM32_I2C_SLAVE_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_I2C == TRUE */

#endif /* HAL_I2C_EXTRA_H */

/** @} */
