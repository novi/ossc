#include "pcm5122.h"
#include "PCM51xx.h"

static PCM51xx pcm5122(0x9c >> 1);

void pcm5122_init()
{
    pcm5122.begin(PCM51xx::SAMPLE_RATE_44_1K, PCM51xx::BITS_PER_SAMPLE_16);
}

uint8_t pcm5122_get_powerstate()
{
    return pcm5122.getPowerState();
}

void pcm5122_set_volume(uint8_t value)
{
    pcm5122.setVolume(value);
}

