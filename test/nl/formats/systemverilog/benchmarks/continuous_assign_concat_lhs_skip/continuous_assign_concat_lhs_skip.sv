// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_assign_concat_lhs_skip(
  input logic a,
  input logic b,
  output logic y0,
  output logic y1
);
  // Concatenation LHS is not currently lowered by resolveExpressionNet.
  assign {y1, y0} = {a, b};
endmodule
