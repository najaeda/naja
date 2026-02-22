// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module unary_not_binary_supported_top(
  input logic a,
  input logic b,
  output logic y_nand,
  output logic y_nor,
  output logic y_xnor
);
  assign y_nand = ~(a & b);
  assign y_nor = ~(a | b);
  assign y_xnor = ~(a ^ b);
endmodule
