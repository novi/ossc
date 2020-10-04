#include "soundbox.h"
#include "pcm5122.h"
#include "tlv320adc3100.h"
#include "sysconfig.h" // for printf
#include "mcu.h"
#include <unistd.h>

void soundbox_init()
{
    uint8_t ret = pcm5122_init();
    if (ret) {
        printf("pcm5122 init failure %d\n", ret);
        printf("pcm5122 power state = 0x%02x\n", pcm5122_get_powerstate());
    }
    if (!mcu_init()) {
        printf("mcu init failure\n");
    }

    pcm5122_set_volume(255); // 48 = 0dB, 255 = inf dB(mute)
    printf("audio adc init = 0x%02x\n", tlv320adc_init());
    printf("sound box (dac, adc, mcu) initialized.\n");
}

void soundbox_loop_tick()
{
    static uint8_t counter = 0;
    counter++;
    printf("sb loop tick %d\n", counter);
    usleep(1000*1000); // 1sec

    mcu_send_data(counter);


    printf("audio sample rate = 0x%02x, bck = 0x%04x, power = 0x%02x, clock = 0x%02x, clock error = 0x%02x, mute = 0x%02x\n",
    pcm5122_get_current_samplerate(),
    pcm5122_get_bck_state(),
    pcm5122_get_powerstate(),
    pcm5122_get_clock_state(),
    pcm5122_get_clock_error(),
    pcm5122_get_mute_state() );

    pcm5122_set_volume(80);
}