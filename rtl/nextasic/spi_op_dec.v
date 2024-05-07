//
// Copyright 2021-24 Yusuke Ito
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

`default_nettype none

module SPIOpDecoder(
	input wire [7:0] op, // 3 bytes
	input wire op_valid,
	output reg is_keyboard_data,
	output reg is_mouse_data,
	output reg is_mic_data
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


module SPIKeyboardMux(
	input wire [16:0] spi_keyboard_data,
	input wire spi_keyboard_data_valid,
	input wire [16:0] nonadb_keyboard_data,
	input wire nonadb_keyboard_data_valid,
	
	output reg [16:0] keyboard_data,
	output wire keyboard_data_valid
);

	assign keyboard_data_valid = spi_keyboard_data_valid | nonadb_keyboard_data_valid;
	
	always@ (*) begin
		if (spi_keyboard_data_valid)
			keyboard_data = spi_keyboard_data;
		else
			keyboard_data = nonadb_keyboard_data;
	end
	

endmodule
