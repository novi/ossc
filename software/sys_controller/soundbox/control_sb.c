#include <string.h>
#include <unistd.h>
#include "control_sb.h"
#include "alt_types.h"
#include "controls.h"
#include "sysconfig.h" // for printf
#include "av_controller.h"
#include "menu.h"
#include "lcd.h"

// ----- original OSSC definition

extern char menu_row1[LCD_ROW_LEN+1], menu_row2[LCD_ROW_LEN+1];
extern mode_data_t video_modes[];
extern avmode_t cm;

extern avconfig_t tc;

extern alt_u8 update_cur_vm, vm_edit;
extern alt_u16 tc_sampler_phase;

alt_u32 remote_code = 0;
alt_u8 remote_rpt = 0, remote_rpt_prev = 0;
alt_u32 btn_code = 0, btn_code_prev = 0;
alt_u16 rc_keymap[REMOTE_MAX_KEYS] = {0};

extern alt_u8 menu_active;
extern volatile osd_regs *osd;
extern alt_u8 profile_sel, profile_sel_menu;
extern volatile sc_regs *sc;

// -----

#define KEYSCAN_NORMALKEY_MASK 0x8000
#define KEYSCAN_NORMALKEY_BREAK 0x0080
#define KEYSCAN_ONLY_MODIFIER 0x0080
#define KEYSCAN_MODIFIER_ALT_LEFT 0x2000
#define KEYSCAN_MODIFIER_ALT_RIGHT 0x4000

#define KEYSCAN_KEY_SPACE 0x38
#define KEYSCAN_KEY_MINUS 0x1d
#define KEYSCAN_KEY_PLUS 0x1c
#define KEYSCAN_KEY_1 0x4a
#define KEYSCAN_KEY_2 0x4b
#define KEYSCAN_KEY_3 0x4c
#define KEYSCAN_KEY_4 0x4d
#define KEYSCAN_KEY_5 0x50
#define KEYSCAN_KEY_9 0x1f
#define KEYSCAN_KEY_0 0x20
#define KEYSCAN_KEY_P 0x08
#define KEYSCAN_KEY_O 0x07
#define KEYSCAN_KEY_L 0x2d

#define KEYSCAN_KEY_BACKSPACE 0x1b
#define KEYSCAN_KEY_ESC 0x49
#define KEYSCAN_KEY_ENTER 0x2a
#define KEYSCAN_KEY_LEFT 0x09
#define KEYSCAN_KEY_RIGHT 0x10
#define KEYSCAN_KEY_UP 0x16
#define KEYSCAN_KEY_DOWN 0x0f

static uint8_t lm_mode_menu_active = 0;

menucode_id menu_code = NO_ACTION;

void sb_keyboard_changed(uint16_t scancode)
{
    sc_status_reg sc_status;
    sc_status2_reg sc_status2;

    alt_u8 pt_only = 0;
    alt_u8* pmcfg_ptr[] = { &pt_only, &tc.pm_240p, &tc.pm_384p, &tc.pm_480i, &tc.pm_480p, &tc.pm_480p, &tc.pm_1080i };
    alt_u8 valid_pm[] = { 0x1, 0x1f, 0x3, 0xf, 0x3, 0x3, 0x3 };

    if ( (scancode & KEYSCAN_NORMALKEY_MASK) &&
         !(scancode & KEYSCAN_NORMALKEY_BREAK) ) {
        // normal key pressed
        uint8_t keyscancode = scancode & 0x7f;
        if (menu_active) {
            printf("normal key 0x%02x\n", keyscancode);
        }

        if (menu_active && lm_mode_menu_active) {
            uint8_t i = 0xff;
            switch (keyscancode) {
                case KEYSCAN_KEY_1: i = 0; break;
                case KEYSCAN_KEY_2: i = 1; break;
                case KEYSCAN_KEY_3: i = 2; break;
                case KEYSCAN_KEY_4: i = 3; break;
                case KEYSCAN_KEY_5: i = 4; break;
            }
            if (i < 5) {
                printf("key %d pressed\n", i+1);
                if ((1<<i) & valid_pm[video_modes[cm.id].group]) {
                    *pmcfg_ptr[video_modes[cm.id].group] = i;
                } else {
                    sniprintf(menu_row2, LCD_ROW_LEN+1, "%ux unsupported", i+1);
                    lcd_write_menu();
                    usleep(500000);
                }
            }
            lm_mode_menu_active = 0;
            lcd_write_status();
        } else if (menu_active) {
            switch (keyscancode) {
            case KEYSCAN_KEY_SPACE:
                sc_status = sc->sc_status;
                sc_status2 = sc->sc_status2;
                sniprintf(menu_row1, LCD_ROW_LEN+1, "Prof.%u %9s", profile_sel, video_modes[cm.id].name);
                if (cm.sync_active) {
                    //fpga_v_hz_x100 = (100*TVP_EXTCLK_HZ)/IORD_ALTERA_AVALON_PIO_DATA(PIO_8_BASE);
                    /*sniprintf(menu_row2, LCD_ROW_LEN+1, "%4lu%c%c  %3lu.%.2luHz", (((fpga_status & 0x7ff)+1)<<fpga_ilace)+fpga_ilace,
                                                                                fpga_ilace ? 'i' : 'p',
                                                                                ((fpga_status >> 16) & 0x3) ? '*' : ' ',
                                                                                fpga_v_hz_x100/100,
                                                                                fpga_v_hz_x100%100);*/
                    sniprintf(menu_row2, LCD_ROW_LEN+1, "%4lu%c%c  %lu", (unsigned long)((sc_status.vmax+1)<<sc_status.interlace_flag)+sc_status.interlace_flag,
                                                                        sc_status.interlace_flag ? 'i' : 'p',
                                                                        sc_status.fpga_vsyncgen ? '*' : ' ',
                                                                        (unsigned long)sc_status2.pcnt_frame);
                }
                osd->osd_config.menu_active = 1;
                lcd_write_menu();
                break;
            case KEYSCAN_KEY_9: tc.sl_mode = (tc.sl_mode < SL_MODE_MAX) ? (tc.sl_mode + 1) : 0; break;
            case KEYSCAN_KEY_0: tc.sl_type = (tc.sl_type < SL_TYPE_MAX) ? (tc.sl_type + 1) : 0; break;
            case KEYSCAN_KEY_MINUS: tc.sl_str = tc.sl_str ? (tc.sl_str - 1) : 0; break;
            case KEYSCAN_KEY_PLUS: tc.sl_str = (tc.sl_str < SCANLINESTR_MAX) ? (tc.sl_str + 1) : SCANLINESTR_MAX; break;
            case KEYSCAN_KEY_P:
                video_modes[cm.id].sampler_phase = (video_modes[cm.id].sampler_phase < SAMPLER_PHASE_MAX) ? (video_modes[cm.id].sampler_phase + 1) : 0;
                update_cur_vm = 1;
                if (cm.id == vm_edit)
                    tc_sampler_phase = video_modes[vm_edit].sampler_phase;
                break;
            case KEYSCAN_KEY_O: video_modes[cm.id].sampler_phase = video_modes[cm.id].sampler_phase ? (video_modes[cm.id].sampler_phase - 1) : SAMPLER_PHASE_MAX;
                update_cur_vm = 1;
                if (cm.id == vm_edit)
                    tc_sampler_phase = video_modes[vm_edit].sampler_phase;
                break;
            case KEYSCAN_KEY_L:
                strncpy(menu_row1, "Linemult mode:", LCD_ROW_LEN+1);
                strncpy(menu_row2, "press 1-5", LCD_ROW_LEN+1);
                lcd_write_menu();
                lm_mode_menu_active = 1;

                if (video_modes[cm.id].group > GROUP_1080I) {
                    printf("WARNING: Corrupted mode (id %d)\n", cm.id);
                }

                // osd->osd_config.menu_active = 0;
                // lcd_write_status();
                // menu_active = 0;
                break;
            case KEYSCAN_KEY_BACKSPACE: menu_code = PREV_MENU; break;
            case KEYSCAN_KEY_ESC: menu_code = PREV_MENU; break;
            case KEYSCAN_KEY_ENTER: menu_code = OPT_SELECT; break;
            case KEYSCAN_KEY_LEFT: menu_code = VAL_MINUS; break;
            case KEYSCAN_KEY_RIGHT: menu_code = VAL_PLUS; break;
            case KEYSCAN_KEY_UP: menu_code = PREV_PAGE; break;
            case KEYSCAN_KEY_DOWN: menu_code = NEXT_PAGE; break;
            }
        }

    } else if ( (scancode & KEYSCAN_ONLY_MODIFIER) && 
        (scancode & KEYSCAN_MODIFIER_ALT_LEFT) &&
        (scancode & KEYSCAN_MODIFIER_ALT_RIGHT) ) {
        
        menu_active = !menu_active;
        osd->osd_config.menu_active = menu_active;
        profile_sel_menu = profile_sel;

        lm_mode_menu_active = 0;

        printf("only modifier, left and right alt, menu active = %d\n", menu_active);

        if (menu_active)
            display_menu(1);
        else
            lcd_write_status();
    }
}


