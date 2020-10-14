`default_nettype none

module SPIOpDecoder(
	input wire [7:0] op, // 3 bytes
	input wire op_valid,
	output reg is_keyboard_data,
	output reg is_mouse_data,
	output reg is_mic_data,
);

	always@ (*) begin
		is_keyboard_data = 0;
		is_mouse_data = 0;
		is_mic_data = 0;
		if (op_valid)
			casex (op)
				8'h1: begin
					is_keyboard_data = 1;
				end
				8'h2: begin
					is_mouse_data = 1;
				end
				8'h3: begin
					is_mic_data = 1;
				end
				default: begin
				end
			endcase
	end

endmodule