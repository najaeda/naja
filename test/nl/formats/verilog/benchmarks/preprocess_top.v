// Verilog file that requires preprocessing (define/ifdef/include).
`include "preprocess_defs.vh"
`define MAKE_CHILD

module top(input [`RANGE] a, output [`RANGE] y);
`ifdef MAKE_CHILD
  child u0(.a(a), .y(y));
`else
  assign y = a;
`endif
endmodule
