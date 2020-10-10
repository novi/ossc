#ifndef PCM5122_H
#define PCM5122_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t pcm5122_init(); // if returns 0, it is success
uint8_t pcm5122_get_powerstate();
void pcm5122_set_volume(uint8_t l, uint8_t r);
uint8_t pcm5122_get_current_samplerate();
uint16_t pcm5122_get_bck_state();
uint8_t pcm5122_get_clock_state();
uint8_t pcm5122_get_clock_error();
uint8_t pcm5122_get_mute_state();

#ifdef __cplusplus
};
#endif


#endif // PCM5122_H