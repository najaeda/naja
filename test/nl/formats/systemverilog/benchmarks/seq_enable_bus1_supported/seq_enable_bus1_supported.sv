module seq_enable_bus1_supported(
  input logic clk,
  input logic rst,
  input logic [0:0] en,
  input logic [7:0] d,
  output logic [7:0] q
);
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) q <= d;
    else q <= q;
  end
endmodule
