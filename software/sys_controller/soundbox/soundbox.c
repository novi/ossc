#include "soundbox.h"
#include "pcm5122.h"
#include "tlv320adc3100.h"
#include "sysconfig.h" // for printf
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "mcu.h"
#include <unistd.h>
#include "sys/alt_timestamp.h"
#include "control_sb.h"

extern alt_u16 sys_ctrl;

// sys_ctrl bits for SoundBox
#define CONTROL_LED_FROM_SOFTWARE_BIT   (1<<4)
#define LED_ON_BIT                      (1<<5)
#define ENABLE_NEXT_KEYBOARD_BIT        (1<<6)


// pio input bits
#define SCANCODE_MASK            0x0000ffff
#define VOLUME_MASK             0xfff00000
#define VOLUME_MASK_SHIFT       (20)
#define IS_MUTED_BIT            0x00010000

#define TIMESTAMP_FREQ (27) // 27Mhz (FPGA clock)
#define TIMESTAMP_UNIT_MS (1000000)

static uint8_t nextvol_to_pcmvol(uint8_t nextvol)
{
    uint16_t v = ((nextvol*(255-48))/43)+48;
    return v;
}

void soundbox_init()
{
    uint8_t ret = pcm5122_init();
    if (ret) {
        printf("pcm5122 init failure %d\n", ret);
        printf("pcm5122 power state = 0x%02x\n", pcm5122_get_powerstate());
    }
    pcm5122_set_volume(255, 255); // 48 = 0dB, 255 = inf dB(mute)

    if (!mcu_init()) {
        printf("mcu init failure\n");
    }

    ret = tlv320adc_init();
    if (ret) {
        printf("audio adc init = 0x%02x\n", ret);
    }
    printf("sound box (dac, adc, mcu) initialized.\n");
}

static void soundbox_loop_ping_tick()
{
    // send ping packet
    static uint8_t counter = 0;
    counter++;
    printf("sb loop tick %d\n", counter);
    mcu_send_data(counter);

    printf("audio sample rate = 0x%02x, bck = 0x%04x, power = 0x%02x, clock = 0x%02x, clock error = 0x%02x, mute = 0x%02x\n",
    pcm5122_get_current_samplerate(),
    pcm5122_get_bck_state(),
    pcm5122_get_powerstate(),
    pcm5122_get_clock_state(),
    pcm5122_get_clock_error(),
    pcm5122_get_mute_state() );

    tlv320adc_set_mic_volume(16); // in db
    pcm5122_set_volume(60, 60); // TODO: volume

    uint32_t pioin = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);
    uint16_t scancode = pioin & SCANCODE_MASK; // 0xMMSS (M=modifier, S=scancode)
    uint16_t volume = pioin >> VOLUME_MASK_SHIFT;
    uint8_t vol_l = (volume >> 6) & 0x3f; // 0(=0 db) to 43(=inf db), 0xfff=invalid(or not available)
    uint8_t vol_r = (volume) & 0x3f;
    uint8_t is_muted = (pioin & IS_MUTED_BIT) ? 1 : 0;
    printf("latest scancode = 0x%04x, volume L=%d, R=%d, muted?=%d\n", scancode, vol_l, vol_r, is_muted);

    uint8_t adc_status = tlv320adc_get_raw_status();
    int8_t adc_gain = tlv320adc_get_mic_gain();
    printf("adc raw status = 0x%02x, mic gain = %d\n", adc_status, adc_gain);
    

    // printf("timestamp = %ld, freq= %d, sizeof timestamp = %d\n", alt_timestamp(), alt_timestamp_freq(), sizeof(alt_timestamp_type));
}

static alt_timestamp_type latest_timestamp = 0;
uint16_t latest_scancode = 0;

void soundbox_loop_tick()
{
    uint32_t pioin = IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE);

    uint16_t scancode = pioin & SCANCODE_MASK; // NeXT key scancode
    if (latest_scancode != scancode) {
        latest_scancode = scancode;
        printf("key scancode changed 0x%04x\n", scancode);
        sb_keyboard_changed(scancode);
    }

    // invoke ping_tick() if necessary
    alt_timestamp_type current = alt_timestamp();
    if (current == 0) {
        alt_timestamp_start();
        soundbox_loop_ping_tick();
        latest_timestamp = 1;
    } else {
        if (latest_timestamp == 0) {
            soundbox_loop_ping_tick();
            latest_timestamp = alt_timestamp();
        } else {
            if ( current - latest_timestamp > ((1000*TIMESTAMP_UNIT_MS)/TIMESTAMP_FREQ) ) {
                soundbox_loop_ping_tick();
                latest_timestamp = alt_timestamp();
            }
        }
    }    
}

void update_enable_next_keyboard(uint8_t enabled)
{
    if (enabled) {
        sys_ctrl |= ENABLE_NEXT_KEYBOARD_BIT; // bit set
    } else {
        sys_ctrl &= ~(ENABLE_NEXT_KEYBOARD_BIT); // bit clear
    }
    IOWR_ALTERA_AVALON_PIO_DATA(PIO_0_BASE, sys_ctrl);
}