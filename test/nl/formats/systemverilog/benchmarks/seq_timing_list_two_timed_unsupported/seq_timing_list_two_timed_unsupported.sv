// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_list_two_timed_unsupported(
  input logic clk,
  input logic rst,
  input logic d,
  output logic q
);
  // Two timed statements in the same statement list:
  // the second one should trigger unsupported list timing extraction.
  always begin
    @(posedge clk) if (rst) q <= 1'b0;
    else q <= d;
    @(posedge clk) q <= d;
  end
endmodule
