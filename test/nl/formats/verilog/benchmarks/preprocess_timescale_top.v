// Verilog file that requires preprocessing (`timescale).
`timescale 1ns / 1ps

module timescale_top(input a, output y);
  assign y = a;
endmodule
