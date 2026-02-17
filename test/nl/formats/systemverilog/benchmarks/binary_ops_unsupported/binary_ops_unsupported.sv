// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module binary_ops_unsupported_top(
  input logic a,
  input logic b,
  output logic y_add,
  output logic y_sub,
  output logic y_mul,
  output logic y_div,
  output logic y_mod,
  output logic y_eq,
  output logic y_ne,
  output logic y_case_eq,
  output logic y_case_ne,
  output logic y_ge,
  output logic y_gt,
  output logic y_le,
  output logic y_lt,
  output logic y_wild_eq,
  output logic y_wild_ne,
  output logic y_land,
  output logic y_lor,
  output logic y_impl,
  output logic y_leqv,
  output logic y_lshl,
  output logic y_lshr,
  output logic y_ashl,
  output logic y_ashr,
  output logic y_pow
);
  assign y_add = a + b;
  assign y_sub = a - b;
  assign y_mul = a * b;
  assign y_div = a / b;
  assign y_mod = a % b;
  assign y_eq = a == b;
  assign y_ne = a != b;
  assign y_case_eq = a === b;
  assign y_case_ne = a !== b;
  assign y_ge = a >= b;
  assign y_gt = a > b;
  assign y_le = a <= b;
  assign y_lt = a < b;
  assign y_wild_eq = a ==? b;
  assign y_wild_ne = a !=? b;
  assign y_land = a && b;
  assign y_lor = a || b;
  assign y_impl = a -> b;
  assign y_leqv = a <-> b;
  assign y_lshl = a << b;
  assign y_lshr = a >> b;
  assign y_ashl = a <<< b;
  assign y_ashr = a >>> b;
  assign y_pow = a ** b;
endmodule
