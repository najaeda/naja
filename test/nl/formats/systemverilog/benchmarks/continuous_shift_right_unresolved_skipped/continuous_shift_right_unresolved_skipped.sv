// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_shift_right_unresolved_skipped_top(
  input logic [1:0][3:0] a,
  input logic idx,
  output logic [3:0] y
);
  // Value expression is not directly resolvable to a net in createLogicalRightShiftAssign.
  assign y = a[idx] >> 1;
endmodule
