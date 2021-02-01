`timescale 1ns/1ns
`default_nettype none

module DataSync #(
	parameter W = 4
	) (
	input wire in_clk,
	input wire [W-1:0] in_data,
	input wire in_data_valid,

	input wire out_clk,
	output reg [W-1:0] out_data,
	output reg out_data_valid = 0,
	input wire out_data_retrieved
);

	reg [W-1:0] tmp;
	reg has_data = 0;
	wire has_data_;
	FF2SyncP has_data__(has_data, out_clk, has_data_);
	reg data_retrived = 0;
	wire data_retrived_;
	FF2SyncN data_retrived__(data_retrived, in_clk, data_retrived_);

	always@ (posedge in_clk) begin
		if (!has_data && in_data_valid) begin
			tmp <= in_data;
			has_data <= 1; // TODO: data loss
		end else if (data_retrived_) begin
			has_data <= 0;
		end
	end
	
	always@ (negedge out_clk) begin
		if (has_data_ && !out_data_valid && !data_retrived) begin
			out_data <= tmp;
			out_data_valid <= 1;
		end else if (out_data_valid && !data_retrived && out_data_retrieved) begin
			out_data_valid <= 0;
			data_retrived <= 1;
		end else if (!has_data_ && data_retrived) begin
			data_retrived <= 0;
		end
	end

endmodule


module test_DataSync_Fast_Slow;

	reg in_clk = 0;
	reg [3:0] in_data = 0;
	reg in_data_valid = 0;
	reg out_clk = 0;
	reg out_data_retrieved = 0;
	
	wire [3:0] out_data;
	wire out_data_valid;
	
	parameter IN_CLOCK = 10;
	parameter OUT_CLOCK = IN_CLOCK*100;

	DataSync sync(
		in_clk,
		in_data,
		in_data_valid,
		out_clk,
		out_data,
		out_data_valid,
		out_data_retrieved
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;
	
	
	initial begin
		@(posedge out_data_valid);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
	end
	
	initial begin
		#(IN_CLOCK*10);
		in_data = 4'd1;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*10);
		in_data = 4'd2; // this data will be lost
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*1000);
		
		
		in_data = 4'd3;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*1000);
		$stop;
	end
	
endmodule

module test_DataSync_Slow_Fast;

	reg in_clk = 0;
	reg [3:0] in_data = 0;
	reg in_data_valid = 0;
	reg out_clk = 0;
	reg out_data_retrieved = 0;
	
	wire [3:0] out_data;
	wire out_data_valid;
	
	
	parameter OUT_CLOCK = 10;
	parameter IN_CLOCK = OUT_CLOCK*100;

	DataSync sync(
		in_clk,
		in_data,
		in_data_valid,
		out_clk,
		out_data,
		out_data_valid,
		out_data_retrieved
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;
	
	initial begin
		@(posedge out_data_valid);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
	end
	
	initial begin
		#(IN_CLOCK*10);
		in_data = 4'd1;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		
		in_data = 4'd2; // this data will be lost
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*10);
		
		in_data = 4'd3;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		#(IN_CLOCK*10);
		
		$stop;
	end
	
endmodule
