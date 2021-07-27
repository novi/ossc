#ifndef MCU_STM32_H_
#define MCU_STM32_H_

#include <inttypes.h>

uint8_t mcu_init();
void mcu_send_data_test(uint8_t data);
uint8_t mcu_update_control(uint8_t useSpeaker);

#endif // MCU_STM32_H_