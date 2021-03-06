/*
 * log.h
 *
 *  Created on: Jan 2, 2016
 *      Author: mori
 */

#ifndef LOG_H_
#define LOG_H_

#include "stm32f4xx.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define DEBUG_LOG 0

#if DEBUG_LOG
#define LOG_DEBUG LOG
#else
#define LOG_DEBUG LOG_DUMMY
#endif

uint8_t LOG_INIT(USART_TypeDef *usart, uint32_t baudrate);
void LOG(const char *__msg, ...);
void LOG1(const char *__msg, ...);
void LOG_ARRAY1(const uint8_t *data, int size);
void LOG_ARRAY(const uint8_t *data, int size);
void LOG_DUMMY(const char *__msg, ...);

#ifdef __cplusplus
}
#endif


#endif /* LOG_H_ */
