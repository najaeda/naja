module seq_concat_lhs_skipped(
  input logic clk,
  input logic rst,
  output logic [3:0] q,
  output logic [3:0] r
);
  // Unsupported extraction case: concatenation LHS cannot be resolved to a single net.
  always_ff @(posedge clk)
    if (rst) {q, r} <= 8'h00;
endmodule
