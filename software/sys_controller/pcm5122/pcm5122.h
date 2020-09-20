#ifndef PCM5122_H
#define PCM5122_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t pcm5122_init();
uint8_t pcm5122_get_powerstate();
void pcm5122_set_volume(uint8_t value);

#ifdef __cplusplus
};
#endif


#endif // PCM5122_H