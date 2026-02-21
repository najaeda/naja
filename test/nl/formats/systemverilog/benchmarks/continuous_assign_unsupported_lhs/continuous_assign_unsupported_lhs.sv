// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_assign_unsupported_lhs(
  input logic a,
  output string y
);
  // Unsupported LHS type; continuous-assign lowering skips unresolved lhs net.
  assign y = a;
endmodule
