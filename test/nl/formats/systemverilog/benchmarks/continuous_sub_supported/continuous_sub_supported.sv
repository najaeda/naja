// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_sub_supported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  input logic [4:0] lzc_b_result,
  input logic [5:0] shift_a,
  output logic [3:0] y_sub,
  output logic [3:0] y_dec,
  output logic [1:0] y_small,
  output logic [5:0] y_concat_sub
);
  assign y_sub = a - b;
  assign y_dec = a - 'd1;
  assign y_small = b[1:0] - 2'b01;
  assign y_concat_sub = {1'b0, lzc_b_result} - shift_a;
endmodule
