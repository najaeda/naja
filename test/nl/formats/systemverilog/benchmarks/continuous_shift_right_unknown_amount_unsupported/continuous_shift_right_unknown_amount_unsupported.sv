// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_shift_right_unknown_amount_unsupported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  // Non-constant shift amount with unknown value forces dynamic-shift amount resolution failure.
  assign y = a >>> 1'bx;
endmodule
