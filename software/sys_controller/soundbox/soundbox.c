#include "soundbox.h"
#include "pcm5122.h"
#include "tlv320adc3100.h"
#include "sysconfig.h" // for printf

void soundbox_init()
{
    if (!pcm5122_init()) {
        printf("pcm5122 init failure\n");
        printf("pcm5122 power state = 0x%02x\n", pcm5122_get_powerstate());
    }    

    // TODO:
    pcm5122_set_volume(48); // 48 = 0dB, 255 = inf dB(mute)
    printf("audio adc init = 0x%02x\n", tlv320adc_init());
}