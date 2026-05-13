// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_reset_only_supported(
  input logic clk,
  input logic rst,
  output logic [7:0] q
);
  // Reset-only chain: no default branch.
  always_ff @(posedge clk)
    if (rst) q <= 8'h00;
endmodule
