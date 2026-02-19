// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_add_non_increment (
  output reg [7:0] out,
  input wire [7:0] a,
  input wire [7:0] b,
  input wire clk,
  input wire reset
);
always_ff @(posedge clk)
if (reset) begin
  out = 8'b0;
end else begin
  out = a + b;
end
endmodule
