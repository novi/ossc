#include "mcu.h"
#include <unistd.h>
#include "sysconfig.h" // for printf
#include "system.h"
#include "i2c_opencores.h"
#include "mcu_i2c_structure.h"

#define MCU_I2C_ADDR (0xc8>>1)

uint8_t mcu_init()
{
    return 1;
}

uint8_t mcu_send_recv_data(uint8_t data1, uint8_t data2)
{
    printf("mcu send data = 0x%x, 0x%x\n", data1, data2);
    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 0);
    I2C_write(I2CA_BASE, data1, 0);
    I2C_write(I2CA_BASE, data2, 1);
    usleep(1000*10); // TODO: need?

    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 1); // read
    uint8_t recvData = I2C_read(I2CA_BASE, 1); // read with stop cond
    printf("mcu recv data = 0x%x\n", recvData);
    return recvData;
}

uint8_t mcu_update_control(uint8_t useSpeaker)
{
    uint8_t recvData = mcu_send_recv_data( ((useSpeaker ? 1 : 0 ) << MCU_CONTROL_BIT_USE_SPEAKER), 0x5a);
    return 0;
}

void mcu_send_data_test(uint8_t data)
{
    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 0);
    I2C_write(I2CA_BASE, data, 0);
    // I2C_write(I2CA_BASE, data+1, 0);

    I2C_write(I2CA_BASE, data+1, 1);
    usleep(1000*10);

    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 1); // read
    uint8_t recvData = I2C_read(I2CA_BASE, 1); // read with stop cond
    printf("mcu test recv data = %d\n", recvData);
}