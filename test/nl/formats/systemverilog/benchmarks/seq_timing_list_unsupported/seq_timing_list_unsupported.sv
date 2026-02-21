// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_list_unsupported(
  input logic clk,
  input logic rst,
  input logic d,
  output logic q
);
  // Unsupported by constructor: statement-list wrapper (multiple statements)
  // while extracting a timed sequential statement.
  always begin
    if (rst) q <= 1'b0;
    else q <= d;
    q <= d;
  end
endmodule
