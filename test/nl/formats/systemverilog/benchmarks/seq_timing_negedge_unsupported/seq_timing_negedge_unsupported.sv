// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_negedge_unsupported(
  input logic clk,
  input logic d,
  output logic q
);
  always_ff @(negedge clk) begin
    q <= d;
  end
endmodule
