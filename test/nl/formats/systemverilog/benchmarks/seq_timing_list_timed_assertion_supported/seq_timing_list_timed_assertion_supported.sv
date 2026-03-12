// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_list_timed_assertion_supported(
  input logic clk,
  input logic rst,
  input logic d,
  output logic q
);
  // Multi-item statement list:
  //   1) timed statement (kept)
  //   2) ignorable immediate assertion (skipped)
  always begin
    @(posedge clk) if (rst) q <= 1'b0;
    else q <= d;
    assert (1'b1);
  end
endmodule
