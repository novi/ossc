`default_nettype none

module Microphone(
	input wire clk, // mon clk
	input wire record_start,
	input wire record_stop,

	input wire bclk, // i2s BCLK
	input wire audio_data_in, // i2s audio data
	input wire lrck, // i2s LRCK

	output reg [31:0] mic_data = 0,
	output wire mic_data_valid,
	input wire mic_data_retrieved,
	output wire [1:0] mic_debug
);

	reg record_active = 0;
	reg record_active_delay_start = 0;
	reg [4:0] record_active_delay = 0;
	
	assign mic_debug[0] = record_active;

	always@ (posedge clk) begin
		if (record_stop) begin
			record_active <= 0;
			record_active_delay_start <= 0;
			// TODO: send last available mic data
		end else begin
			if (record_start) begin
				record_active_delay_start <= 1;
				record_active_delay <= 0;
			end else if (record_active_delay_start) begin
				if (record_active_delay == 5'd10) begin // delay to first soundin(mic data) packet
					record_active <= 1;
					record_active_delay_start <= 0;
				end else
					record_active_delay <= record_active_delay + 1'b1;
			end
		end
	end
	
	wire [7:0] ulawout;
	reg [15:0] audio_data = 0; // 16 bit audio
	LIN2MLAW lin2ulaw(audio_data[15:15-12], ulawout);

	reg audio_data_available = 0;
	wire audio_data_available_;
	FF2SyncN audio_data_available__(audio_data_available, clk, audio_data_available_);
	
	reg audio_data_retrieved = 0;

	reg mic_data_filled = 0;
	//reg wait_first = 0;
	assign mic_data_valid = record_active & mic_data_filled; //(record_active | wait_first) & mic_data_filled; // record_active & mic_data_filled;
	reg record_active_p = 0;
	reg [1:0] send_counter = 0; // 0 to 3
	assign mic_debug[1] = mic_data_valid;
	
	always@ (posedge clk) begin
		record_active_p <= record_active;
		
		if (!record_active_p && record_active) begin
			// start
			// TODO: no need this, no need delay to first packet
			mic_data <= 32'hffffffff; // silence sound
			mic_data_filled <= 1;
			send_counter <= 0;
		end else if (mic_data_filled && mic_data_retrieved) begin
			mic_data_filled <= 0;
		end else if (!mic_data_filled) begin
			if (audio_data_available_ && !audio_data_retrieved) begin
				case (send_counter)
					2'd0: mic_data[31:24] <= ulawout;
					2'd1: mic_data[23:16] <= ulawout;
					2'd2: mic_data[15:8] <= ulawout;
					2'd3: begin
						mic_data[7:0] <= ulawout;
						mic_data_filled <= 1;
					end
				endcase
				send_counter <= send_counter + 1'd1;
				audio_data_retrieved <= 1;
			end else if (!audio_data_available_ && audio_data_retrieved) begin
				audio_data_retrieved <= 0;
			end
		end
	end
	
	reg [4:0] recv_counter = 0; // 0 to 31

	reg lrck_p = 0;	
	always@ (posedge bclk) begin
		lrck_p <= lrck;
	
		if (lrck_p != lrck && lrck == 0) begin
			// Left channel
			recv_counter <= 0;
			audio_data <= 0;
		end else if (recv_counter < 16) begin
			recv_counter <= recv_counter + 1'd1;
		end
		
		if (recv_counter == 16) begin
			// audio data valid
			audio_data_available <= 1;
		end else begin
			audio_data_available <= 0;
			audio_data[15:0] <= {audio_data[14:0], audio_data_in};
		end
	end


endmodule

`timescale 1ns/1ns

// TODO: update edge
module test_Microphone;
	reg mon_clk = 0;
	reg record_start = 0;
	reg record_stop = 0;
	reg bclk = 0;
	reg audio_data_in = 0;
	reg lrck = 0;
	
	wire [31:0] mic_data;
	wire mic_data_valid;
	
	reg mic_data_retrieved = 0;
	
	
	wire [1:0] mic_debug;
	
	parameter MON_CLOCK = 200;
	parameter AUDIO_CLOCK = MON_CLOCK*10; // BCLK

	// localparam PACKET_READY = 16'b5a;

	Microphone microphone(
		mon_clk,
		record_start,
		record_stop,
		bclk,
		audio_data_in,
		lrck,
		mic_data,
		mic_data_valid,
		mic_data_retrieved,
		mic_debug
	);
	
	always #(MON_CLOCK/2) mon_clk = ~mon_clk;
	
	always #(AUDIO_CLOCK/2) bclk = ~bclk;
	
	task I2SSend(
		input [15:0] lch,
		input [15:0] rch
	);
		integer i;
		begin
			lrck = 1;
			repeat(5) @(negedge bclk);
			lrck = 0;
			@(negedge bclk);
			for(i = 0; i < 16; i = i + 1) begin
				audio_data_in = lch[15-i];
				@(negedge bclk);
			end
			audio_data_in = 0;
			repeat(5) @(negedge bclk);
			lrck = 1;
			@(negedge bclk);
			for(i = 0; i < 16; i = i + 1) begin
				audio_data_in = rch[15-i];
				@(negedge bclk);
			end
			repeat(5) @(negedge bclk);
		end
	endtask
	
	initial begin
		forever begin
			@(posedge mic_data_valid);
			@(posedge mon_clk);
			@(posedge mon_clk);
			@(posedge mon_clk) mic_data_retrieved = 1;
			@(posedge mon_clk) mic_data_retrieved = 0;
		end
	end
	
	initial begin	
		#(MON_CLOCK*5);
		@(negedge mon_clk) record_start = 1;
		@(negedge mon_clk) record_start = 0;
		#(MON_CLOCK*5);
		
		I2SSend(16'h5a5a, 16'ha5a5);
		I2SSend(16'ha5a5, 16'h5a5a);
		I2SSend(16'h0001, 16'h0001);
		I2SSend(16'hffff, 16'hffff);
		I2SSend(16'h9696, 16'h9696);
		I2SSend(16'haaaa, 16'haaaa);

		#(AUDIO_CLOCK*1000);
		$stop;
	end
	
endmodule

