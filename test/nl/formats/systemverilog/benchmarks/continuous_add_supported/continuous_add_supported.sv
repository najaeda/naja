// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_add_supported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  output logic [3:0] y_sum,
  output logic [3:0] y_inc,
  output logic [1:0] y_count
);
  assign y_sum = a + b;
  assign y_inc = a + 'd1;
  assign y_count = b[1] + b[0];
endmodule
