#include "soundbox.h"
#include "pcm5122.h"
#include "tlv320adc3100.h"
#include "sysconfig.h" // for printf
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "mcu.h"
#include <unistd.h>

// sys_ctrl bits for SoundBox
#define CONTROL_LED_FROM_SOFTWARE   (1<<4)
#define LED_ON                      (1<<5)

// pio input bits
#define KEYCODE_MASK             0x0000ffff

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

    pcm5122_set_volume(60);

    uint32_t pioin = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);
    printf("latest keycode = 0x%04x\n", pioin & KEYCODE_MASK);
}