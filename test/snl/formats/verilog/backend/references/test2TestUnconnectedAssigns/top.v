module top();
wire [-2:2] bus;

assign bus[-2] = 1'b0;

assign bus[0] = 1'b1;

assign bus[2] = 1'b0;
endmodule //top
