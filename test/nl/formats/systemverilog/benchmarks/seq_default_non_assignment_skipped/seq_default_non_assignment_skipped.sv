module seq_default_non_assignment_skipped(
  input logic clk,
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported extraction case: default branch is not a single assignment.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else begin
      q <= d;
      q <= d;
    end
  end
endmodule
