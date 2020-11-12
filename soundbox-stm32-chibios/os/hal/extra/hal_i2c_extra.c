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
 * @file    hal_i2c.c
 * @brief   I2C Driver code.
 *
 * @addtogroup I2C
 * @{
 */
#include "hal.h"
#include "hal_i2c_extra.h"

#if (HAL_USE_I2C == TRUE) || defined(__DOXYGEN__)

#if STM32_I2C_SLAVE_ENABLE

/**
 * @brief   Listen I2C bus for address match.
 * @details Use 7 bit address (10 bit,dual and general call address dosn't implement yet) .
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] addr      slave device address
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 *
 * @notapi
 */
msg_t i2cSlaveMatchAddress(I2CDriver *i2cp, i2caddr_t  i2cadr)
{
  osalDbgCheck((i2cp != NULL) && (i2cadr != 0x00));
  chSysLock();
  msg_t result = i2c_lld_matchAddress(i2cp, i2cadr);
  chSysUnlock();
  return result;
}

/**
 * @brief   Receive data via the I2C bus as slave and call handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[out] rxbuf    pointer to the receive buffer
 * @param[in] rxbytes   size of receive buffer
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2cSlaveOnReceive(I2CDriver *i2cp, i2c_slave_receive_callback_t callback, uint8_t *rxbuf, size_t rxbytes)
{
  msg_t rdymsg;

  osalDbgCheck((i2cp != NULL) && (callback != NULL) &&
		       (rxbytes > 0U) && (rxbuf != NULL));

  osalDbgAssert(i2cp->state == I2C_READY, "not ready");
  osalSysLock();
  i2cp->errors = I2C_NO_ERROR;
  i2cp->state = I2C_ACTIVE_RX;
  i2cp->rxcb = callback;
  i2cp->rxbuf = rxbuf;
  i2cp->rxbytes = rxbytes;
  rdymsg = i2c_lld_slave_on_receive(i2cp, rxbuf, rxbytes);

  if (rdymsg == MSG_TIMEOUT) {
	i2cp->state = I2C_LOCKED;
  }
  else {
	i2cp->state = I2C_READY;
  }

  osalSysUnlock();
  return rdymsg;
}

/**
 * @brief   Set request handler.
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] callback  handler, the function to be called when master request data;
 *
 * @return              nothing.
 *
 * @notapi
 */
void i2cSlaveOnRequest(I2CDriver *i2cp, i2c_slave_transmit_callback_t callback)
{

  /* Check the parameters */
  osalDbgCheck((i2cp != NULL) && (callback != NULL));
  chSysLock();
  i2c_lld_onRequest(i2cp, callback);
  chSysUnlock();
}

/**
 * @brief   Transmits data via the I2C bus as slave.
 * @details Call this function when Master request data (in request handler)
 *
 * @param[in] i2cp      pointer to the @p I2CDriver object
 * @param[in] txbuf     pointer to the transmit buffer
 * @param[in] txbytes   number of bytes to be transmitted
 *                      .
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more I2C errors occurred, the errors can
 *                      be retrieved using @p i2cGetErrors().
 * @retval MSG_TIMEOUT  if a timeout occurred before operation end. <b>After a
 *                      timeout the driver must be stopped and restarted
 *                      because the bus is in an uncertain state</b>.
 *
 * @notapi
 */
msg_t i2cSlaveStartTransmission(I2CDriver *i2cp, const uint8_t *txbuf, size_t txbytes)
{
  msg_t rdymsg;

  osalDbgCheck((i2cp != NULL) && (txbytes > 0U)
			                      && (txbuf != NULL));

  osalDbgAssert(i2cp->state == I2C_READY, "not ready");
  osalSysLock();
  i2cp->errors = I2C_NO_ERROR;
  i2cp->state = I2C_ACTIVE_TX;
  rdymsg = i2c_lld_slave_start_transmission(i2cp, txbuf, txbytes);
  if (rdymsg == MSG_TIMEOUT) {
    i2cp->state = I2C_LOCKED;
  }
  else {
    i2cp->state = I2C_READY;
  }

  osalSysUnlock();
  return rdymsg;
}


#endif /* STM32_I2C_SLAVE_ENABLE */


#endif /* HAL_USE_I2C == TRUE */

/** @} */
