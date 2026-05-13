// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module compatible_net_bus_reuse_top (
  output reg [3:0] out,
  input wire enable,
  input wire clk,
  input wire reset
);
  logic [3:0] inc_out;

  always_ff @(posedge clk)
    if (reset) begin
      out = 4'b0;
    end else if (enable) begin
      out++;
    end
endmodule
