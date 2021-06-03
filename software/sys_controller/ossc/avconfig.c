//
// Copyright (C) 2015-2019  Markus Hiienkari <mhiienka@niksula.hut.fi>
//
// This file is part of Open Source Scan Converter project.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <string.h>
#include "system.h"
#include "avconfig.h"
#include "av_controller.h"
#include "altera_avalon_pio_regs.h"
#include "tvp7002.h"

#define DEFAULT_ON              1

extern mode_data_t video_modes[], video_modes_default[];
extern alt_u8 update_cur_vm;

// Target configuration
avconfig_t tc;

// Default configuration
const avconfig_t tc_default = {
    .l3_mode = 1,
    .pm_240p = 1,
    .pm_384p = 1,
    .pm_480i = 1,
    .pm_1080i = 1,
    .tvp_hpll2x = DEFAULT_ON,
    .sync_vth = DEFAULT_SYNC_VTH,
    .linelen_tol = DEFAULT_LINELEN_TOL,
    .vsync_thold = DEFAULT_VSYNC_THOLD,
    .sync_lpf = DEFAULT_SYNC_LPF,
    .pre_coast = DEFAULT_PRE_COAST,
    .post_coast = DEFAULT_POST_COAST,
    .sl_altern = 1,
#ifdef ENABLE_AUDIO
    .audio_dw_sampl = 0, // DEFAULT_ON,
    .tx_mode = TX_HDMI_RGB,
    .audio_gain = AUDIO_GAIN_0DB,
#endif
    .col = {
        .r_f_gain = DEFAULT_FINE_GAIN,
        .g_f_gain = DEFAULT_FINE_GAIN,
        .b_f_gain = DEFAULT_FINE_GAIN,
        .r_f_off = DEFAULT_FINE_OFFSET,
        .g_f_off = DEFAULT_FINE_OFFSET,
        .b_f_off = DEFAULT_FINE_OFFSET,
        .c_gain = DEFAULT_COARSE_GAIN,
    },
    .link_av = AV_LAST,
    .clamp_offset = SIGNED_NUMVAL_ZERO,
    .alc_h_filter = DEFAULT_ALC_H_FILTER,
    .alc_v_filter = DEFAULT_ALC_V_FILTER,
    .soundbox_volume_max = SB_VOLUME_DB_AMOUNT/2,
    
};

int set_default_avconfig()
{
    memcpy(&tc, &tc_default, sizeof(avconfig_t));
#ifndef ENABLE_AUDIO
    tc.tx_mode = (IORD_ALTERA_AVALON_PIO_DATA(PIO_1_BASE) & HDMITX_MODE_MASK) ? TX_DVI : TX_HDMI_RGB;
#endif

    memcpy(video_modes, video_modes_default, VIDEO_MODES_SIZE);
    update_cur_vm = 1;

    return 0;
}
