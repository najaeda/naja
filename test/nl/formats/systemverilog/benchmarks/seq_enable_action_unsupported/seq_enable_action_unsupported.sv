module seq_enable_action_unsupported(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] a,
  input logic [7:0] b,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported enable action: a + b is not an increment of q.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) q <= a + b;
    else q <= d;
  end
endmodule
