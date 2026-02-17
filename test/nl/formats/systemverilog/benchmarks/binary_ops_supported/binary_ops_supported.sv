// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module binary_ops_supported_top(
  input logic a,
  input logic b,
  output logic y_and,
  output logic y_or,
  output logic y_xor,
  output logic y_xnor
);
  assign y_and = a & b;
  assign y_or = a | b;
  assign y_xor = a ^ b;
  assign y_xnor = a ~^ b;
endmodule
