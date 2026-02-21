module seq_reset_action_unsupported(
  input logic clk,
  input logic rst,
  input logic [7:0] a,
  input logic [7:0] b,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported reset action: a + b is not an increment of q.
  always_ff @(posedge clk) begin
    if (rst) q <= a + b;
    else q <= d;
  end
endmodule
