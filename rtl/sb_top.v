module NextSoundBox (
    input from_kb, // and BTN0
    output to_kb,
    
    input mon_clk,
    input to_mon,
    output from_mon,
    
    output spdif_led0,
    
    input mc_sck,
    input mc_mosi,
    input mc_ss,
    output mc_miso,
    
    input mic_bclk,
    input mic_lrck,
    input mic_data,
    
    input audio_mclk,
    output audio_bclk,
    output audio_lrck,
    output audio_data,
	
	output [15:0] latest_keycode,
	output latest_keycode_valid
);


    // dummy
    // assign to_kb = from_kb;
    // assign from_mon = mon_clk | to_mon;
    assign spdif_led0 = from_mon;
    assign mc_miso = mc_sck | mc_mosi | mc_ss;
    // assign audio_data = mic_bclk | mic_lrck | mic_data | audio_mclk;
    // assign audio_bclk = audio_mclk;
    // assign audio_lrck = audio_data;
    
    wire [39:0] in_data;
    wire data_recv;
    Receiver receiver(
        mon_clk,
        to_mon,
        in_data,
        data_recv
    );
    
    Divider#(.DIVISOR(8), .W(3)) sck_div( // generate BCLK
        audio_mclk,
        audio_bclk
    );
    
    wire is_audio_sample, audio_starts, audio_22khz, end_audio_sample, all_1_packet, power_on_packet_R1, keyboard_led_update,
         attenuation_data_valid;
    wire [7:0] attenuation_data;
    OpDecoder op_decoder(
        in_data[39:16],
        data_recv,
        is_audio_sample,
        audio_starts,
        audio_22khz,
        end_audio_sample,
        all_1_packet,
        power_on_packet_R1,
        keyboard_led_update,
        attenuation_data_valid,
        attenuation_data
    );
    
    wire audio_sample_request_mode, audio_sample_request_tick;
    I2SSender i2s(
        mon_clk,
        is_audio_sample,
        in_data[31:0],
        audio_starts,
        end_audio_sample,
        audio_22khz,
        audio_sample_request_mode,
        audio_sample_request_tick,
        audio_bclk,
        audio_lrck,
        audio_data
    );
    
    wire [15:0] keyboard_data;
    wire keyboard_data_ready, is_mouse_data;
    Keyboard keyboard(
        mon_clk,
        keyboard_led_update,
        in_data[17:16],
        keyboard_data_ready,
        is_mouse_data,
        keyboard_data,
        from_kb,
        to_kb,
        //debug_test_pins[4:0]
    );
    
    wire is_muted, db_val_valid;
    wire [5:0] lch_db;
    wire [5:0] rch_db;
    wire [7:0] att_debug_out;
    Attenuation att(
        mon_clk,
        attenuation_data_valid,
        attenuation_data,
        is_muted,
        lch_db,
        rch_db,
        db_val_valid
    );
    
    wire [39:0] out_data;
    wire out_valid, power_on_packet_S1, data_loss;
    
    Delay #(.DELAY(14000), .W(14)) power_on_packet_delay( // 2.8ms delay
        mon_clk,
        power_on_packet_R1,
        0,
        power_on_packet_S1
    );
    
    assign latest_keycode = keyboard_data;
    assign latest_keycode_valid = keyboard_data_ready & (~is_mouse_data);
    
    OpEncoder op_enc(
        power_on_packet_S1,
        keyboard_data_ready,
        is_mouse_data,
        keyboard_data,
        out_data,
        out_valid
    );

    Sender sender(
        mon_clk,
        out_data,
        out_valid,
        audio_sample_request_mode,
        audio_sample_request_tick,
        from_mon,
        data_loss
    );

endmodule