// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_mul_zero_supported_top(
  input logic [3:0] a,
  output logic [3:0] y
);
  // Exercise createMultiplyAssign path where all partial products are zero.
  assign y = a * 4'b0000;
endmodule
