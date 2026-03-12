// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_binary_expression_fallbacks_unsupported_top(
  input logic [1:0][3:0] a,
  input logic idx,
  input logic [3:0] b,
  input logic [2:0] shamt,
  output logic [3:0] y_shift,
  output logic [3:0] y_mul,
  output logic [3:0] y_sub,
  output logic [1:0] y_eq_bus,
  output logic [1:0] y_ne_bus
);
  // Exercise fallback unsupported-reporting paths for specialized continuous-assign handlers.
  assign y_shift = a[idx] <<< shamt;
  assign y_mul = a[idx] * b;
  assign y_sub = a[idx] - b;
  assign y_eq_bus = (a[idx] == b);
  assign y_ne_bus = (a[idx] != b);
endmodule
