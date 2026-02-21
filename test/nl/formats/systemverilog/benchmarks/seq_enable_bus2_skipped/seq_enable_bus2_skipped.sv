// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_enable_bus2_skipped(
  input logic clk,
  input logic rst,
  input logic [1:0] en,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Constructor currently expects a single-bit condition net.
  // `en` is 2-bit, so the sequential chain is skipped.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en) q <= d;
    else q <= q;
  end
endmodule
