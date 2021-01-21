#ifndef TLV320ADC_H
#define TLV320ADC_H

#include <inttypes.h>


uint8_t tlv320adc_init();
uint8_t tlv320adc_get_raw_status();
int8_t tlv320adc_get_mic_gain();
void tlv320adc_set_mic_volume(uint8_t db);

#endif // TLV320ADC_H