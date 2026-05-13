// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_ne_supported_top(
  input logic a,
  input logic b,
  input logic [3:0] x,
  input logic [3:0] y,
  input logic [1:0] fu,
  output logic y_ne1,
  output logic y_ne4,
  output logic y_ne2,
  output logic y_ne_enum
);
  typedef enum logic [1:0] {
    ALU = 2'b00,
    CVXIF = 2'b11
  } fu_t;

  assign y_ne1 = (a != b);
  assign y_ne4 = (x != y);
  assign y_ne2 = (x[2:1] != 2'b10);
  assign y_ne_enum = (fu_t'(fu) != CVXIF);
endmodule
