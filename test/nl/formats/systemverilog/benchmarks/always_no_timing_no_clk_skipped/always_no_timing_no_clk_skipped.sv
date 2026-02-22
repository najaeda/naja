// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module always_no_timing_no_clk_skipped(
  input logic rst,
  input logic [7:0] d,
  output logic [7:0] q
);
  // No timing control and no `clk` net to fall back to.
  always begin
    if (rst) q <= 8'h00;
    else q <= d;
  end
endmodule
