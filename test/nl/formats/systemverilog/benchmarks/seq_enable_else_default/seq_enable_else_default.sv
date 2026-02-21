module seq_enable_else_default(
  input logic clk,
  input logic rst,
  input logic en,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Supported sequential chain with explicit default action:
  // if (rst) ... else if (en) ... else ...
  always_ff @(posedge clk) begin
    if (rst) q <= 0;
    else if (en) q <= q + 1;
    else q <= d;
  end
endmodule
