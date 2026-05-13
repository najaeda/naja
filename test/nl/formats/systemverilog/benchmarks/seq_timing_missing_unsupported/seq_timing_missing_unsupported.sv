// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_timing_missing_unsupported(
  input logic clk,
  input logic rst,
  input logic d,
  output logic q
);
  // Unsupported by constructor: no timing control (@...) on sequential statement.
  always if (rst) q <= 1'b0;
  else q <= d;
endmodule
