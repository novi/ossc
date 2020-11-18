#include "mcu.h"
#include <unistd.h>
#include "sysconfig.h" // for printf
#include "system.h"
#include "i2c_opencores.h"

#define MCU_I2C_ADDR (0xc8>>1)

uint8_t mcu_init()
{
    return 1;
}

void mcu_send_data(uint8_t data)
{
    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 0);
    I2C_write(I2CA_BASE, data, 0);
    // I2C_write(I2CA_BASE, data+1, 0);

    I2C_write(I2CA_BASE, data+1, 1);
    usleep(1000*10);

    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 1); // read
    uint8_t recvData = I2C_read(I2CA_BASE, 1); // read and stop cond
    printf("mcu recv data = %d\n", recvData);
}