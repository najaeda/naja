module top(input [-4:-4] in, output [6:6] out);
wire feedtru;

assign feedtru = in[-4];
assign out[6] = feedtru;

endmodule //top
