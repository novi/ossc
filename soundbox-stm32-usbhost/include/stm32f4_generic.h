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

void MX_GPIO_Init(void);
void MX_I2C2_Init(void);
void MX_SPI1_Init(void);


#ifdef __cplusplus
}
#endif

#endif /* __STM32F4_GENERIC_H */
