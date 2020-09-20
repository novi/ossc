#include "tlv320adc3100.h"
#include "system.h"
#include "sysconfig.h"
#include "i2c_opencores.h"

#define TLV320ADDR (0x30>>1)

uint8_t tlv320adc_reg_read(uint8_t reg)
{
    // TODO: page

    I2C_start(I2CA_BASE, TLV320ADDR, 0);
    I2C_write(I2CA_BASE, reg, 0);

    I2C_start(I2CA_BASE, TLV320ADDR, 1);
    return I2C_read(I2CA_BASE, 1);
}

uint8_t tlv320_set_page(uint8_t page)
{
    I2C_start(I2CA_BASE, TLV320ADDR, 0);
    I2C_write(I2CA_BASE, 0, 0);
    I2C_write(I2CA_BASE, page, 1);
}

uint8_t tlv320adc_init()
{
    tlv320_set_page(0);
    return tlv320adc_reg_read(20); // ADC AOSR
}