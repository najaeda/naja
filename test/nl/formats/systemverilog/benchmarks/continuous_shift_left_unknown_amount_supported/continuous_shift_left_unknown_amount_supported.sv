// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_shift_left_unknown_amount_supported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  // Unknown literal shift amount lowers to zero in the 2-state SNL model.
  assign y = a <<< 1'bx;
endmodule
