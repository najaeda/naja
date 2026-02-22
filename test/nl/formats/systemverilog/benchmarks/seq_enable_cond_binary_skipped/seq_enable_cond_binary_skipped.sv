// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_enable_cond_binary_skipped(
  input logic clk,
  input logic rst,
  input logic en_a,
  input logic en_b,
  input logic [7:0] d,
  output logic [7:0] q
);
  // Unsupported condition shape for enable path: binary expression is not a resolvable net.
  always_ff @(posedge clk) begin
    if (rst) q <= 8'h00;
    else if (en_a & en_b) q <= d;
    else q <= q;
  end
endmodule
