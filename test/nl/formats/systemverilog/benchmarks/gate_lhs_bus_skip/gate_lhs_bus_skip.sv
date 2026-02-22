// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module gate_lhs_bus_skip_top(
  input logic [1:0] a,
  input logic [1:0] b,
  output logic [1:0] y
);
  assign y = a & b;
endmodule
