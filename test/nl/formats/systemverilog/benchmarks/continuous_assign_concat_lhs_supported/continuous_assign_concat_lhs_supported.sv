// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_assign_concat_lhs_supported(
  input logic a,
  input logic b,
  output logic y0,
  output logic y1
);
  // Concatenation LHS exercises multi-target continuous assignment lowering.
  assign {y1, y0} = {a, b};
endmodule
