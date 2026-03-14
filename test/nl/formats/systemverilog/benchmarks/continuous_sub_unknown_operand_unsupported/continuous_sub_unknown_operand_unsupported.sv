// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_sub_unknown_operand_unsupported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  assign y = a - 4'bx001;
endmodule
