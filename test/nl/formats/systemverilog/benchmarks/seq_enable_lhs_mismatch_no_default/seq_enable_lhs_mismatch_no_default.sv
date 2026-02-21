module seq_enable_lhs_mismatch_no_default(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] d,
  output logic [7:0] q,
  output logic [7:0] r
);
  // Unsupported extraction case: enable branch assigns a different LHS.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) r <= d;
  end
endmodule
