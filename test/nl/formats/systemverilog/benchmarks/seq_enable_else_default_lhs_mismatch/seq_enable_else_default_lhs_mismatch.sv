module seq_enable_else_default_lhs_mismatch(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] d,
  output logic [7:0] q,
  output logic [7:0] r
);
  // Unsupported in chain extraction: default branch assigns a different LHS.
  always_ff @(posedge clk) begin
    if (rst) q <= 0;
    else if (en) q <= q + 1;
    else r <= d;
  end
endmodule
