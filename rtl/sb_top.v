module NextSoundBox (
    input clk27, // FPGA clk,
    input hw_reset_n, // FPGA reset

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
    output latest_keycode_valid,
    output is_muted,
    output [11:0] volume_db, // Lch, Rch
    output volume_db_valid,

    input enable_next_keyboard
);


    // dummy
    // assign spdif_led0 = from_mon;
    
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
    
    wire is_audio_sample, audio_starts, audio_22khz, audio_22khz_repeats, end_audio_sample, all_1_packet, power_on_packet_R1, keyboard_led_update,
         attenuation_data_valid, mic_start, mic_stop;
    wire [7:0] attenuation_data;
    wire debug_audio_control_changed;
    OpDecoder op_decoder(
        in_data[39:16],
        data_recv,
        is_audio_sample,
        audio_starts,
        audio_22khz,
        audio_22khz_repeats,
        end_audio_sample,
        all_1_packet,
        power_on_packet_R1,
        keyboard_led_update,
        attenuation_data_valid,
        attenuation_data,
        mic_start,
        mic_stop,
        debug_audio_control_changed
    );
    
    wire audio_sample_request_mode, audio_sample_request_underrun, audio_sample_request_tick;
    I2SSender i2s(
        mon_clk,
        is_audio_sample,
        in_data[31:0],
        audio_starts,
        end_audio_sample,
        audio_22khz,
        audio_sample_request_mode,
        audio_sample_request_underrun,
        audio_sample_request_tick,
        audio_bclk,
        audio_lrck,
        audio_data
    );
    
    // ----- SPI slave
    wire [7:0] spi_data;
    wire spi_data_valid;
    
    wire [23:0] spi_buf;
    wire spi_buf_valid;
    
    SPI_Slave #(.SPI_MODE(1)) spi_slave(
        hw_reset_n,
        clk27,
        spi_data_valid,
        spi_data,
        1'b1,
        8'h00,
        mc_sck,
        mc_miso,
        mc_mosi,
        mc_ss
    );
    
    SPIReceiver spi_recv(
        clk27,
        spi_data,
        spi_data_valid,
        mc_ss,
        spi_buf,
        spi_buf_valid
    );
    
    wire spi_is_keyboard_data, spi_is_mouse_data, spi_is_mic_data;
    wire [16:0] spi_keyboard_data;
    assign spi_keyboard_data = {spi_is_mouse_data, spi_buf[7:0], spi_buf[15:8]};
    SPIOpDecoder spi_opdec(
        spi_buf[23:16],
        spi_buf_valid,
        spi_is_keyboard_data,
        spi_is_mouse_data,
        spi_is_mic_data
    );
    
    
    // ----- keyboard
    
    wire keyboard_led_update_s;
    wire [1:0] keyboard_led_data_s;

    wire [16:0] keyboard_data_nonadb; 
    wire keyboard_data_ready, is_mouse_data;
    assign keyboard_data_nonadb[16] = is_mouse_data;
    Keyboard keyboard(
        clk27, // driven by FPGA clock
        keyboard_led_update_s,
        keyboard_led_data_s,
        keyboard_data_ready,
        is_mouse_data,
        keyboard_data_nonadb[15:0], // keyboard_data
        from_kb,
        to_kb
        //debug_test_pins[4:0]
    );
    
    wire [16:0] keyboard_data_s; // FPGA(system) clock side
    wire keyboard_data_valid_s;
    wire is_mouse_s;
    assign is_mouse_s = keyboard_data_s[16];
    SPIKeyboardMux keyboard_mux(
        spi_keyboard_data,
        spi_is_keyboard_data | spi_is_mouse_data,
        keyboard_data_nonadb,
        keyboard_data_ready,
        keyboard_data_s,
        keyboard_data_valid_s
    );
    assign latest_keycode = keyboard_data_s[15:0];
    assign latest_keycode_valid = keyboard_data_valid_s & (~is_mouse_s);
    
    wire [16:0] keyboard_data_n; // next hardware (mon_clk) side
    wire keyboard_data_ready_n;
    
    wire keyboard_data_retrieved, out_data_retrieved;
    DataSync #(.W(17)) keyboard_data_sync ( // mon_clk domain to FPGA clock domain
        clk27,
        keyboard_data_s,
        keyboard_data_valid_s & (is_mouse_s | enable_next_keyboard | (!keyboard_data_s[15]) ), // non normal key always enabled, mouse always enabled
        mon_clk,
        keyboard_data_n,
        keyboard_data_ready_n,
        keyboard_data_retrieved & out_data_retrieved // for keyboard out_data_retrieved
    );
    
    DataSync #(.W(2)) keyboard_led_sync ( // FPGA clock domain to mon_clk domain
        mon_clk,
        in_data[17:16],
        keyboard_led_update,
        clk27,
        keyboard_led_data_s,
        keyboard_led_update_s,
        1 // out_data_retrieved
    );
    
    Attenuation att(
        mon_clk,
        attenuation_data_valid,
        attenuation_data,
        is_muted,
        volume_db[11:6],
        volume_db[5:0],
        volume_db_valid
    );

    wire [31:0] mic_data_bus;
    wire is_mic_data, mic_data_retrieved;
    wire [1:0] mic_debug;
    Microphone microphone(
        mon_clk,
        mic_start,
        mic_stop,
        mic_bclk,
        mic_data,
        mic_lrck,
        mic_data_bus,
        is_mic_data,
        mic_data_retrieved & out_data_retrieved,
        mic_debug
    );
    
    wire [39:0] out_data;
    wire out_valid, power_on_packet_S1;
    
    Delay #(.DELAY(14000), .W(14)) power_on_packet_delay( // 2.8ms delay
        mon_clk,
        power_on_packet_R1,
        0,
        power_on_packet_S1
    );


    OpEncoder op_enc(
        power_on_packet_S1,
        keyboard_data_ready_n,
        keyboard_data_n[16], // is_mouse_data
        keyboard_data_n[15:0], // keyboard_data
        keyboard_data_retrieved,
        is_mic_data,
        mic_data_bus,
        mic_data_retrieved,
        out_data,
        out_valid
    );

    // TODO: debug
    // assign spdif_led0 = out_data_retrieved;
    // assign spdif_led0 = mic_debug[0];
    
    reg cur_audio_22khz_repeats = 0;
    assign spdif_led0 = cur_audio_22khz_repeats;
    always@ (posedge mon_clk) begin
        if (data_recv && mic_start)
            cur_audio_22khz_repeats <= 1;
        else if (data_recv && mic_stop)
            cur_audio_22khz_repeats <= 0;
    end
    
    // assign spdif_led0 = audio_sample_request_mode;
    
    wire data_loss;
    Sender sender(
        mon_clk,
        out_data,
        out_valid,
        audio_sample_request_mode,
        audio_sample_request_underrun,
        audio_sample_request_tick,
        from_mon,
        data_loss,
        out_data_retrieved
    );

endmodule