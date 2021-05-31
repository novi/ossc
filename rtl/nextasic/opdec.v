`default_nettype none

module OpDecoder(
	input wire [23:0] op, // 3 bytes
	input wire op_valid,
	output reg is_audio_sample,
	output reg audio_starts,
	output reg audio_22khz,
	output reg audio_22khz_repeats,
	output reg end_audio_sample,
	output reg all_1_packet, // can be used for entire reset
	output reg power_on_packet_R1,
	output reg keyboard_led_update,
	output reg attenuation_data_valid,
	output reg [7:0] attenuation_data,
	output reg mic_start, // microphone record
	output reg mic_stop,
	output reg debug_audio_control_changed
);

	wire [7:0] data1;
	wire [7:0] data2;
	assign data1 = op[15:8];
	assign data2 = op[7:0];

	always@ (*) begin
		is_audio_sample = 0;
		audio_starts = 0;
		all_1_packet = 0;
		power_on_packet_R1 = 0;
		keyboard_led_update = 0;
		audio_22khz = 0;
		audio_22khz_repeats = 0;
		end_audio_sample = 0;
		attenuation_data_valid = 0;
		attenuation_data = 8'hxx;
		mic_start = 0;
		mic_stop = 0;
		
		debug_audio_control_changed = 0;
		
		if (op_valid) begin
			casex (op)
				24'hc5ef??: begin
					power_on_packet_R1 = 1;
				end
				24'hc500??: begin
					keyboard_led_update = 1;
				end
				24'hc4????: if (data2 == 0) begin
					attenuation_data_valid = 1;
					attenuation_data = data1;
				end
				24'hc7????: begin
					is_audio_sample = 1;
				end
				// 24'h0b????: begin
				24'b00??1011????????????????: begin
					mic_start = 1;
				end
				// 24'h03????: begin
				24'b00??0011????????????????: begin
					mic_stop = 1;
				end
				24'hff????: begin
					all_1_packet = 1;
				end
				default: begin
				end
			endcase
			// TODO: refactor to assign
			casex (op[23:16])
				8'b00??0111: begin
					end_audio_sample = 1;
				end
				8'b00??1111: begin
					audio_starts = 1;
				end
				default: begin
				end
			endcase
			casex (op[23:16])
				8'b00?1?111: begin
					audio_22khz = 1;
					debug_audio_control_changed = 1;
				end
				8'b00?0?111: begin
					debug_audio_control_changed = 1;
				end
				default: begin
				end
			endcase
			casex (op[23:16])
				8'b000??111: begin
					audio_22khz_repeats = 1;
				end
				8'b001??111: begin
					audio_22khz_repeats = 0; // zero fill
				end
				default: begin
				end
			endcase
		end
	end

endmodule
