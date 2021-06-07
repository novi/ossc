#include "tlv320adc3100.h"
#include "system.h"
#include "sysconfig.h"
#include "i2c_opencores.h"
#include <unistd.h> // usleep

void tlv320_set_page(uint8_t page);

#define TLV320ADDR (0x30>>1)
#define GET_PAGE_FROM_REG(r) ((r>>8) & 0xff)

#define REG_SW_RESET 1 // page 0, reg 1
#define REG_CLOCKGEN_MULTI 4
#define REG_PLL_P_R 5
#define REG_PLL_J 6
#define REG_PLL_D_MSB 7
#define REG_PLL_D_LSB 8
#define REG_ADC_NADC 18
#define REG_ADC_MADC 19 
#define REG_ADC_AOSR 20
#define REG_ADC_INTERFACE_1 27
#define REG_ADC_INTERFACE_2 29
#define REG_BCLK_N_DIV 30
#define REG_ADC_FLAG 36
#define REG_I2S_TDM 38
#define REG_ADC_DIGITAL 81
#define REG_ADC_VOLUME 82
#define REG_RIGHT_VOLUME 84
#define REG_RIGHT_AGC_CTRL_1 94
#define REG_RIGHT_AGC_MAX_GAIN 96
#define REG_RIGHT_AGC_APPLIED_GAIN 101

#define REG_MICBIAS ((1 << 8) | 51) // page 1, reg 51
#define REG_R_PGA_55 ((1 << 8) | 55)
#define REG_R_PGA_57 ((1 << 8) | 57)
#define REG_R_PGA_GAIN_60 ((1 << 8) | 60)


static uint8_t currentPage = 0;

uint8_t tlv320adc_reg_read_page(uint8_t page, uint8_t reg)
{
    tlv320_set_page(page);

    I2C_start(I2CA_BASE, TLV320ADDR, 0);
    I2C_write(I2CA_BASE, reg, 0);

    I2C_start(I2CA_BASE, TLV320ADDR, 1);
    return I2C_read(I2CA_BASE, 1);
}

uint8_t tlv320adc_reg_read(uint16_t reg)
{
    return tlv320adc_reg_read_page(GET_PAGE_FROM_REG(reg), reg & 0xff);
}


void tlv320adc_reg_write_page(uint8_t page, uint8_t reg, uint8_t value)
{
    tlv320_set_page(page);

    I2C_start(I2CA_BASE, TLV320ADDR, 0);
    I2C_write(I2CA_BASE, reg, 0);
    I2C_write(I2CA_BASE, value, 1);
}

void tlv320adc_reg_write(uint16_t reg, uint8_t value)
{
    tlv320adc_reg_write_page(GET_PAGE_FROM_REG(reg), reg & 0xff, value);
}

void tlv320_set_page(uint8_t page)
{
    if (currentPage == page) return;

    I2C_start(I2CA_BASE, TLV320ADDR, 0);
    I2C_write(I2CA_BASE, 0, 0);
    I2C_write(I2CA_BASE, page, 1);
    currentPage = page;
}

uint8_t tlv320adc_init()
{
    tlv320adc_reg_write(REG_SW_RESET, 1); // software reset
    usleep(100); // wait for reset
    uint8_t aosr_on_reset = tlv320adc_reg_read(REG_ADC_AOSR);
    // fs=8,000 (Hz)
    // MCLK=22,579,200 (Hz)
    // PLL CLK = 91,728,000
    // ADC CLK = 13,104,000
    // ADC MOD CLK= 936,000 (936khz) (need 64fs=512Khz)
    // ADC FS = 8000
    tlv320adc_reg_write(REG_CLOCKGEN_MULTI, 0x3); // CODEC_CLKIN = PLL_CLK
    tlv320adc_reg_write(REG_PLL_J, 8); // J = 8
    tlv320adc_reg_write(REG_PLL_D_MSB, 0);
    tlv320adc_reg_write(REG_PLL_D_LSB, 125); // D = 125
    tlv320adc_reg_write(REG_PLL_P_R, 0x80 | (2 << 4) |  1); // PLL ON, P = 2, R = 1
    usleep(100); // wait for PLL is stabilized

    tlv320adc_reg_write(REG_ADC_NADC, 0x80 | 7); // NADC = 7, NADC ON
    tlv320adc_reg_write(REG_ADC_MADC, 0x80 | 14); // MADC = 14, MADC ON
    tlv320adc_reg_write(REG_ADC_AOSR, 117); // AOSR = 117
    // MADCxAOSR>=IADC
    // 14*117=1638
    tlv320adc_reg_write(REG_ADC_INTERFACE_1, 0xc); // I2S 16bit, BLCK is out, WCLK is out
    tlv320adc_reg_write(REG_ADC_INTERFACE_2, 0x3); // BDIV_CLKIN = ADC_MOD_CLK
    tlv320adc_reg_write(REG_BCLK_N_DIV, 0x80 | 1); // BCLK OUT divider N = 1 and the divider is ON
    tlv320adc_reg_write(REG_I2S_TDM, 1 << 4); // channel swap enabled
    
    tlv320adc_reg_write(REG_MICBIAS, 1 << 5); // MICBIAS 2.0volts
    tlv320adc_reg_write(REG_R_PGA_55, 0x3c); // differential pair only (0db)
    tlv320adc_reg_write(REG_R_PGA_57, 0x3c); // not connect Lch input to Rch PGA
    tlv320adc_reg_write(REG_R_PGA_GAIN_60, 80); // R PGA is NOT muted, setting gain db(=value/2)

    tlv320adc_reg_write(REG_RIGHT_AGC_CTRL_1, 1 << 7); // Rch AGC is ON
    tlv320adc_reg_write(REG_RIGHT_AGC_MAX_GAIN, 80); // Max gain is 0db (=value/2)

    tlv320adc_reg_write(REG_ADC_DIGITAL, 0x40); // Lch ADC is OFF, Rch ADC is ON
    tlv320adc_reg_write(REG_ADC_VOLUME, 0x80); // Lch ADC is mute, Rch ADC is NOT muted, both 0db
    tlv320adc_reg_write(REG_ADC_VOLUME, 0x80);
    return aosr_on_reset != 0x80; // should be 0x80 on reset
}

uint8_t tlv320adc_get_raw_status()
{
    return tlv320adc_reg_read(REG_ADC_FLAG);
}

int8_t tlv320adc_get_mic_gain()
{
    return (int8_t)tlv320adc_reg_read(REG_RIGHT_AGC_APPLIED_GAIN);
}

void tlv320adc_set_mic_volume(uint8_t db)
{
    // db should be (-12 to 20)
    tlv320adc_reg_write(REG_RIGHT_VOLUME, db*2); // Rch volume db(=value/2) 
}