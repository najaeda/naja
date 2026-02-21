module seq_enable_iftrue_non_assignment_skipped(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported extraction case: enable if-true branch is not a single assignment.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) begin
      q <= d;
      q <= d;
    end
  end
endmodule
