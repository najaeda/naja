// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module unary_not_binary_unsupported_top(
  input logic a,
  input logic b,
  output logic y
);
  assign y = ~(a + b);
endmodule
