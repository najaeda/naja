// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module binary_ops_supported_top(
  input logic a,
  input logic b,
  input logic [3:0] s,
  output logic y_and,
  output logic y_land,
  output logic y_or,
  output logic y_lor,
  output logic y_xor,
  output logic y_xnor,
  output logic [3:0] y_shr
);
  assign y_and = a & b;
  assign y_land = a && b;
  assign y_or = a | b;
  assign y_lor = a || b;
  assign y_xor = a ^ b;
  assign y_xnor = a ~^ b;
  assign y_shr = s >> 1;
endmodule
