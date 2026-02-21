module seq_enable_else_default_non_assignment(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported in chain extraction: enable else-branch is not a single assignment.
  always_ff @(posedge clk) begin
    if (rst) q <= 0;
    else if (en) q <= q + 1;
    else begin
      q <= d;
      q <= d;
    end
  end
endmodule
