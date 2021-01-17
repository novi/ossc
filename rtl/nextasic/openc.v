`default_nettype none

module OpEncoder(
	input wire power_on_packet_S1,
	
	input wire keyboard_data_ready,
	input wire is_mouse_data,
	input wire [15:0] keyboard_data, // 2 bytes
	output reg keyboard_data_retrieved,
	
	input wire is_mic_data, // microphone record data
	input wire [31:0] mic_data,
	output reg mic_data_retrieved,
	
	output reg [39:0] data,
	output reg data_valid
);

	always@ (*) begin
		data = 40'hxxxxxxxxxx;
		data_valid = 0;
		keyboard_data_retrieved = 0;
		mic_data_retrieved = 0;
		if (power_on_packet_S1) begin
			data = 40'hc671000000; // may be ok 40'hc670000000 as well
			data_valid = 1;
		end else if (is_mic_data) begin
			data[39:32] = 8'hc7;
			data[31:0] = mic_data;
			data_valid = 1;
			mic_data_retrieved = 1;
		end else if (keyboard_data_ready) begin
			data[39:32] = 8'hc6;
			data[31:24] = is_mouse_data ? 8'h01 : 8'h10;
			data[23:16] = 8'h00;
			data[15:0] = keyboard_data;
			data_valid = 1;
			keyboard_data_retrieved = 1;
		end 
	end

endmodule
