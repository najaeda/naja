// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_eq_ne_failure_paths_unsupported_top(
  input logic a,
  output logic y_eq_string,
  output logic y_eq_unknown,
  output logic y_ne_string
);
  string s0;
  string s1;

  // Non-integral operands: width inference for equality cannot proceed.
  assign y_eq_string = (s0 == s1);
  // Integral but unknown RHS literal: bit resolution fails.
  assign y_eq_unknown = (a == 1'bx);
  // Inequality path delegates through equality first and should fail there.
  assign y_ne_string = (s0 != s1);
endmodule
