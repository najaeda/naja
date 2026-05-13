// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module unsupported_symbol_expr(
  input logic a,
  output logic y
);
  shortreal wide_f;
  assign y = wide_f;
endmodule
