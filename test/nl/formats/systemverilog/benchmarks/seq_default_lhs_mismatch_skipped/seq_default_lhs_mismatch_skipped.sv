module seq_default_lhs_mismatch_skipped(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q,
  output logic [7:0] r
);
  // Unsupported extraction case: default branch assigns a different LHS.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else r <= d;
  end
endmodule
