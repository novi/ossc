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

`timescale 1ns/1ns
`default_nettype none

module FF2Sync(
	input wire in,
	input wire out_clk,
	output wire out_data
);

	reg f1, f2;
	
	assign out_data = f2;
	
	always@ (out_clk) begin
		f1 <= in;
		f2 <= f1;
	end
endmodule

module FF2SyncP(
	input wire in,
	input wire out_clk,
	output wire out_data
);

	reg f1, f2;
	
	assign out_data = f2;
	
	always@ (posedge out_clk) begin
		f1 <= in;
		f2 <= f1;
	end
endmodule

module FF2SyncN(
	input wire in,
	input wire out_clk,
	output wire out_data
);

	reg f1, f2;
	
	assign out_data = f2;
	
	always@ (negedge out_clk) begin
		f1 <= in;
		f2 <= f1;
	end
endmodule

module Delay #(
	parameter DELAY = 10,
	parameter W = 4
	) (
	input wire clk,
	input wire in_data,
	input wire reset,
	output wire out_data
);
	reg [W-1:0] counter;
	reg running = 0;
	
	assign out_data = running & (counter == (DELAY+1));
	
	always@ (posedge clk) begin
		if (reset) begin
			running <= 0;
			counter <= 0;
		end else begin
			if (running) begin
				if (counter == (DELAY+1)) begin
					running <= 0;
				end else begin
					counter <= counter + 1'b1;
				end
			end else begin
				if (in_data && !running) begin
					counter <= 0;
					running <= 1;
				end
			end
		end
	end
endmodule

module test_Delay;
	reg clk = 0;
	reg in_data = 0;
	wire out;
	
	parameter CLOCK = 200;

	Delay delay(
		clk,
		in_data,
		0,
		out
	);
	
	always #(CLOCK/2) clk = ~clk;
	
	initial begin	
		#(CLOCK*5);
		@(negedge clk);
		in_data = 1;
		@(negedge clk);
		in_data = 0;
		#(CLOCK*50) $stop;
	end
	
endmodule

module Divider#(
	parameter DIVISOR = 4'd8,
	parameter W = 3
	) (
	input wire clk,
	output wire out
);
	reg[W-1:0] counter = 0;
	
	assign out = counter[2];
	
	always @(posedge clk) begin
		if(counter >= (DIVISOR-1))
			counter <= 0;
		else begin
			counter <= counter + 1'b1;
		end
	end
endmodule

module test_Divider;

	reg clk = 0;
	wire out;
	
	parameter CLOCK = 100;

	Divider div(
		clk,
		out
	);
	
	always #(CLOCK/2) clk = ~clk;
	
	
	initial begin		
		#(CLOCK*50) $stop;
	end
	
endmodule
