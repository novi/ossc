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
    output audio_data
);


    // dummy
    assign to_kb = from_kb;
    assign from_mon = mon_clk | to_mon;
    assign spdif_led0 = 0;
    assign mc_miso = mc_sck | mc_mosi | mc_ss;
    assign audio_data = mic_bclk | mic_lrck | mic_data | audio_mclk;
    assign audio_bclk = audo_mclk;
    assign audio_lrck = audio_data;

endmodule