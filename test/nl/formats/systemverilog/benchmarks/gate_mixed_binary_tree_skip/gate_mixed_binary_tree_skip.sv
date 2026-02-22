// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module gate_mixed_binary_tree_skip(
  input logic a,
  input logic b,
  input logic c,
  output logic y
);
  // Mixed operator tree; only flat same-operator trees are currently lowered.
  assign y = a & (b | c);
endmodule
