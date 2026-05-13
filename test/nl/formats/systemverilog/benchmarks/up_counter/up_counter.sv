// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module up_counter (
  output reg [7:0] out,
  input wire enable,
  input wire clk,
  input wire reset
);
always_ff @(posedge clk)
if (reset) begin
  out = 8'b0;
end else if (enable) begin
  out++;
end
endmodule
