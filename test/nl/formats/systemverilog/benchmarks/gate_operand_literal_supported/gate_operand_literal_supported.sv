// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module gate_operand_literal_supported_top(
  input logic a,
  output logic y
);
  assign y = a & 1'b1;
endmodule
