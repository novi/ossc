#ifndef __STM32F4_GENERIC_H
#define __STM32F4_GENERIC_H

#ifdef __cplusplus
 extern "C" {
#endif
                                              
#include "stm32f4xx_hal.h"

#define USARTx                       	 USART2

#define USARTx_CLK_ENABLE()              __USART2_CLK_ENABLE()
#define USARTx_RX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()
#define USARTx_TX_GPIO_CLK_ENABLE()      __GPIOA_CLK_ENABLE()

#define USARTx_TX_PIN                    GPIO_PIN_2
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART2
#define USARTx_RX_PIN                    GPIO_PIN_3
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART2

typedef enum {
  PIN_IN_USB_FAULT, // PC9
  PIN_OUT_USB_ENABLE, // PC8
  PIN_IN_OSSC_POWER, // PC12
  PIN_OUT_MONOUT_INTERFACE_ENABLE, // PC5, negative logic, open-drain
  PIN_OUT_NEXT_POWERSW, // PC4
  PIN_OUT_SPI_SS, // PA4, negative logic
  PIN_OUT_STATUS_LED, // PB14
} MCU_Pin;

void WriteGPIO(MCU_Pin pin, uint8_t value); // 0 is disable, otherwise enable
uint8_t ReadGPIO(MCU_Pin pin);

void MX_GPIO_Init(void);
void MX_I2C2_Init(void);
void MX_I2C2_DeInit(void);
void MX_SPI1_Init(void);

void clear_i2c_intr_counter();

SPI_HandleTypeDef hspi1;
I2C_HandleTypeDef hi2c2;

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4_GENERIC_H */
