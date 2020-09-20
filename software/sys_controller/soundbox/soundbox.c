#include "soundbox.h"
#include "pcm5122.h"
#include "sysconfig.h" // for printf


void soundbox_init()
{
    pcm5122_init();
    printf("pcm5122 power state = 0x%02x\n", pcm5122_get_powerstate());

    // TODO:
    pcm5122_set_volume(48); // 48 = 0dB, 255 = inf dB(mute)
}