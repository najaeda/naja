// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_shift_right_dynamic_element_select_supported_top(
  input logic [1:0][3:0] a,
  input logic idx,
  output logic [3:0] y
);
  // Dynamic packed-array element select feeds the shift value expression.
  assign y = a[idx] >> 1;
endmodule
