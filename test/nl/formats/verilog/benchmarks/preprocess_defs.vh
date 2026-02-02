// Preprocessor definitions for preprocess_top.v
`define RANGE 1:0

module child(input [`RANGE] a, output [`RANGE] y);
  assign y = a;
endmodule
