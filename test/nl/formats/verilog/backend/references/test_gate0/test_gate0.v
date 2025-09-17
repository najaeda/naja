////////////////////////////////////////////////////////////////////////////////
#IGNORE#
#IGNORE#
#IGNORE#
#IGNORE#
////////////////////////////////////////////////////////////////////////////////

module top();
wire [4:0] busNet;
wire out0Net;
wire out1Net;

and and5(out0Net, busNet[0], busNet[1], busNet[2], busNet[3], busNet[4]);

xor (out1Net, busNet[0], busNet[1], busNet[2], busNet[3], busNet[4]);
endmodule //top
