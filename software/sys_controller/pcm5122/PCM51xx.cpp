/*
  MIT License

  Copyright (c) 2018 Tom Magnier

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <unistd.h>
#include "PCM51xx.h"
#include "system.h"
#include "sysconfig.h"
#include "i2c_opencores.h"

void PCM51xx_selectPage(PCM51xx_* p, PCM51xx::Register address);
void PCM51xx_reset(PCM51xx_* p);
void PCM51xx_setPowerMode(PCM51xx_* p, PCM51xx::PowerMode mode);
uint8_t PCM51xx_readRegister(PCM51xx_* p, PCM51xx::Register address);
void PCM51xx_writeRegister(PCM51xx_* p, PCM51xx::Register address, uint8_t data);


uint8_t PCM51xx_getCurrentSampleRate(PCM51xx_* p)
{
  return PCM51xx_readRegister(p, PCM51xx::Register::DET_FS_SCK_RATIO);
}

uint8_t PCM51xx_getClockState(PCM51xx_* p)
{
  return PCM51xx_readRegister(p, PCM51xx::Register::CLOCK_STATUS);
}

uint8_t PCM51xx_getClockErrorState(PCM51xx_* p)
{
  return PCM51xx_readRegister(p, PCM51xx::Register::CLOCK_ERROR);
}

uint8_t PCM51xx_getMuteState(PCM51xx_* p)
{
  return PCM51xx_readRegister(p, PCM51xx::Register::ANALOG_MUTE_MON);
}

uint16_t PCM51xx_getBckState(PCM51xx_* p)
{
  uint16_t v = (PCM51xx_readRegister(p, PCM51xx::Register::DET_BCK_RATIO_MSB) << 8) & 0xff00;
  v |= (0x00ff & PCM51xx_readRegister(p, PCM51xx::Register::DET_BCK_RATIO_LSB) );
  return v;
}

void PCM51xx_PCM51xx(PCM51xx_* p, uint8_t i2cAddr)
{
  p->_i2cAddr = i2cAddr;
  p->_currentPage = 0xFF;
}


bool PCM51xx_begin_bitDepth(PCM51xx_* p, PCM51xx::BitDepth bps)
{
  //Force a page counter sync between the local variable and the IC
  p->_currentPage = 0xFF;
  PCM51xx_selectPage(p, PCM51xx::RESET);

  // Set correct I2S config
  uint8_t config = 0;
  switch (bps) {
    case PCM51xx::BITS_PER_SAMPLE_16: config = 0x00; break;
    case PCM51xx::BITS_PER_SAMPLE_24: config = 0x02; break;
    case PCM51xx::BITS_PER_SAMPLE_32: config = 0x03; break;
  };
  PCM51xx_writeRegister(p, PCM51xx::I2S_FORMAT, config);

  usleep(10*1000); //Wait for calibration, startup, etc

  if (PCM51xx_getPowerState(p) != PCM51xx::POWER_STATE_RUN) {
    return PCM51xx_getPowerState(p);
  }
  return 0; // success
}

uint8_t PCM51xx_begin(PCM51xx_* handle, PCM51xx::SamplingRate rate, PCM51xx::BitDepth bps)
{
  // return 111;

  //See here : https://e2e.ti.com/support/data_converters/audio_converters/f/64/t/428281
  // for a config example

  // Check that the bit clock (PLL input) is between 1MHz and 50MHz
  uint32_t bckFreq = rate * bps * 2;
  if (bckFreq < 1000000 || bckFreq > 50000000)
    return 254;

  // 24 bits is not supported for 44.1kHz and 48kHz.
  if ((rate == PCM51xx::SAMPLE_RATE_44_1K || rate == PCM51xx::SAMPLE_RATE_48K) && bps == PCM51xx::BITS_PER_SAMPLE_24)
    return 253;

  //Force a page counter sync between the local variable and the IC
  handle->_currentPage = 0xFF;

  //Initialize system clock from the I2S BCK input
  PCM51xx_reset(handle);

  // change to VCOM mode
  PCM51xx_setPowerMode(handle, PCM51xx::PowerMode::POWER_MODE_STANDBY);
  PCM51xx_writeRegister(handle, PCM51xx::Register::VCOM_RAMP_SPEED, 1);
  PCM51xx_writeRegister(handle, PCM51xx::Register::VCOM_POWERDOWN, 0); // VCOM power on
  usleep(20*1000); // wait for VCOM warm up
  PCM51xx_writeRegister(handle, PCM51xx::Register::VCOM_RAMP_SPEED, 0);
  PCM51xx_writeRegister(handle, PCM51xx::Register::OUTPUT_AMPL_TYPE, 1);
  PCM51xx_setPowerMode(handle, PCM51xx::PowerMode::POWER_MODE_ACTIVE);

  // PCM51xx_writeRegister(handle, PCM51xx::IGNORE_ERRORS, 0x1A);  // Disable clock autoset and ignore SCK detection
  // use MCK as PLL clock source

  // PCM51xx_writeRegister(handle, PCM51xx::PLL_CLOCK_SOURCE, 0x10);  // Set PLL clock source to BCK
  PCM51xx_writeRegister(handle, PCM51xx::DAC_CLOCK_SOURCE, 0x30); // Set DAC clock source to MCK
  
  PCM51xx_writeRegister(handle, PCM51xx::DIGITAL_VOLUME_CTRL, 1); // Right channel volume follows left channel setting

  PCM51xx_writeRegister(handle, PCM51xx::DIGITAL_VOLUME_RAMP, 0xff);
  PCM51xx_writeRegister(handle, PCM51xx::DIGITAL_VOLUME_EMERG, 0xf0);

  //PLL configuration
  int p, j, d, r;

  //Clock dividers
  int nmac, ndac, ncp, dosr, idac;

  if (rate == PCM51xx::SAMPLE_RATE_11_025K || rate == PCM51xx::SAMPLE_RATE_22_05K || rate == PCM51xx::SAMPLE_RATE_44_1K)
  {
    //44.1kHz and derivatives.
    //P = 1, R = 2, D = 0 for all supported combinations.
    //Set J to have PLL clk = 90.3168 MHz
    p = 1;
    r = 2;
    j = 90316800 / bckFreq / r;
    d = 0;

    //Derive clocks from the 90.3168MHz PLL
    nmac = 2;
    ndac = 16;
    ncp = 4;
    dosr = 8;
    idac = 1024; // DSP clock / sample rate
  }
  else
  {
    //8kHz and multiples.
    //PLL config for a 98.304 MHz PLL clk
    if (bps == PCM51xx::BITS_PER_SAMPLE_24 && bckFreq > 1536000)
      p = 3;
    else if (bckFreq > 12288000)
      p = 2;
    else
      p = 1;

    r = 2;
    j = 98304000 / (bckFreq / p) / r;
    d = 0;

    //Derive clocks from the 98.304MHz PLL
    switch (rate) {
      case PCM51xx::SAMPLE_RATE_16K: nmac = 6; break;
      case PCM51xx::SAMPLE_RATE_32K: nmac = 3; break;
      default:              nmac = 2; break;
    };

    ndac = 16;
    ncp = 4;
    dosr = 384000 / rate;
    idac = 98304000 / nmac / rate; // DSP clock / sample rate
  }

  // Configure PLL
  // PCM51xx_writeRegister(handle, PCM51xx::PLL_P, p - 1);
  // PCM51xx_writeRegister(handle, PCM51xx::PLL_J, j);
  // PCM51xx_writeRegister(handle, PCM51xx::PLL_D_MSB, (d >> 8) & 0x3F);
  // PCM51xx_writeRegister(handle, PCM51xx::PLL_D_LSB, d & 0xFF);
  // PCM51xx_writeRegister(handle, PCM51xx::PLL_R, r - 1);

  // // Clock dividers
  // PCM51xx_writeRegister(handle, PCM51xx::DSP_CLOCK_DIV, nmac - 1);
  // PCM51xx_writeRegister(handle, PCM51xx::DAC_CLOCK_DIV, ndac - 1);
  // PCM51xx_writeRegister(handle, PCM51xx::NCP_CLOCK_DIV, ncp - 1);
  // PCM51xx_writeRegister(handle, PCM51xx::OSR_CLOCK_DIV, dosr - 1);

  // IDAC (nb of DSP clock cycles per sample)
  // PCM51xx_writeRegister(handle, PCM51xx::IDAC_MSB, (idac >> 8) & 0xFF);
  // PCM51xx_writeRegister(handle, PCM51xx::IDAC_LSB, idac & 0xFF);

  // FS speed mode
  int speedMode;
  if (rate <= PCM51xx::SAMPLE_RATE_48K)
    speedMode = 0;
  else if (rate <= PCM51xx::SAMPLE_RATE_96K)
    speedMode = 1;
  else if (rate <= PCM51xx::SAMPLE_RATE_192K)
    speedMode = 2;
  else
    speedMode = 3;
  
  PCM51xx_writeRegister(handle, PCM51xx::FS_SPEED_MODE, speedMode);

  return PCM51xx_begin_bitDepth(handle, bps);
}

void PCM51xx_setPowerMode(PCM51xx_* p, PCM51xx::PowerMode mode)
{
  PCM51xx_writeRegister(p, PCM51xx::STANDBY_POWERDOWN, mode);
}

void PCM51xx_reset(PCM51xx_* p)
{
  PCM51xx_setPowerMode(p, PCM51xx::POWER_MODE_STANDBY);
  PCM51xx_writeRegister(p, PCM51xx::RESET, 0x11);
  PCM51xx_setPowerMode(p, PCM51xx::POWER_MODE_ACTIVE);
}

PCM51xx::PowerState PCM51xx_getPowerState(PCM51xx_* p)
{
  uint8_t regValue = PCM51xx_readRegister(p, PCM51xx::DSP_BOOT_POWER_STATE);

  return (PCM51xx::PowerState)(regValue & 0x0F);
}

void PCM51xx_setVolume(PCM51xx_* p, uint8_t vol)
{
  PCM51xx_writeRegister(p, PCM51xx::DIGITAL_VOLUME_L, vol);
  // PCM51xx_writeRegister(p, PCM51xx::DIGITAL_VOLUME_R, vol); // right channel follows left channel
}

void PCM51xx_writeRegister(PCM51xx_* p, PCM51xx::Register address, uint8_t data)
{
  PCM51xx_selectPage(p, address);

  I2C_start(I2CA_BASE, p->_i2cAddr, 0);
  I2C_write(I2CA_BASE, address, 0);
  I2C_write(I2CA_BASE, data, 1);
}

uint8_t PCM51xx_readRegister(PCM51xx_* p, PCM51xx::Register address)
{
  PCM51xx_selectPage(p, address);

  I2C_start(I2CA_BASE, p->_i2cAddr, 0);
  I2C_write(I2CA_BASE, address, 0);

  I2C_start(I2CA_BASE, p->_i2cAddr, 1);
  return I2C_read(I2CA_BASE, 1);
}

void PCM51xx_selectPage(PCM51xx_* p, PCM51xx::Register address)
{
  uint8_t page = (address >> 8) & 0xFF;

  if (page != p->_currentPage)
  {
    I2C_start(I2CA_BASE, p->_i2cAddr, 0);
    I2C_write(I2CA_BASE, 0, 0);
    I2C_write(I2CA_BASE, page, 1);

    p->_currentPage = page;
  }
}
