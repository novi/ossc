#include "mcu.h"
#include <unistd.h>
#include "sysconfig.h"
#include "system.h"
#include "i2c_opencores.h"

#define MCU_I2C_ADDR (0x98>>1)

uint8_t mcu_init()
{
    return 1;
}

void mcu_send_data(uint8_t data)
{
    I2C_start(I2CA_BASE, MCU_I2C_ADDR, 0);
    // I2C_write(I2CA_BASE, regaddr, 0);
    I2C_write(I2CA_BASE, data, 1);
}