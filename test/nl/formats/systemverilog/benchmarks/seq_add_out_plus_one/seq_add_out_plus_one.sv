// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_add_out_plus_one (
  output reg [7:0] out,
  input wire clk,
  input wire reset
);
always_ff @(posedge clk)
if (reset) begin
  out = 8'b0;
end else begin
  out = out + 1;
end
endmodule
