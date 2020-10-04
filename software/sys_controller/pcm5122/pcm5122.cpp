#include "pcm5122.h"
#include "PCM51xx.h"

static PCM51xx_ pcm5122;

uint8_t pcm5122_init()
{
    PCM51xx_PCM51xx(&pcm5122, 0x9c >> 1);
    return PCM51xx_begin(&pcm5122, PCM51xx::SAMPLE_RATE_44_1K, PCM51xx::BITS_PER_SAMPLE_16);
}

uint8_t pcm5122_get_powerstate()
{
    return (uint8_t)PCM51xx_getPowerState(&pcm5122);
}

void pcm5122_set_volume(uint8_t value)
{
    PCM51xx_setVolume(&pcm5122, value);
}

uint8_t pcm5122_get_current_samplerate()
{
    return PCM51xx_getCurrentSampleRate(&pcm5122);
}

uint16_t pcm5122_get_bck_state()
{
    return PCM51xx_getBckState(&pcm5122);
}

uint8_t pcm5122_get_clock_state()
{
    return PCM51xx_getClockState(&pcm5122);
}

uint8_t pcm5122_get_clock_error()
{
    return PCM51xx_getClockErrorState(&pcm5122); 
}

uint8_t pcm5122_get_mute_state()
{
    return PCM51xx_getMuteState(&pcm5122);
}