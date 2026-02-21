module seq_decrement_skipped(
  input logic clk,
  input logic rst,
  output logic [7:0] q
);
  // Unsupported extraction case: decrement is not handled as assignment action.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else --q;
  end
endmodule
