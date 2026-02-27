// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_mul_supported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  input logic sign_a,
  input logic sign_b,
  output logic [7:0] y_mul,
  output logic [7:0] y_signed_mul
);
  assign y_mul = a * b;
  assign y_signed_mul =
      $signed({a[3] & sign_a, a}) *
      $signed({b[3] & sign_b, b});
endmodule
