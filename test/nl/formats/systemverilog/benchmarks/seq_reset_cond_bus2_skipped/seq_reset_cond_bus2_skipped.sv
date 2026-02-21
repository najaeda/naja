module seq_reset_cond_bus2_skipped(
  input logic clk,
  input logic [1:0] rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported reset condition width: reset condition is 2-bit.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
