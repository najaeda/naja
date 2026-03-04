// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_shift_left_supported_top(
  input logic [7:0] a,
  input logic [2:0] shamt,
  output logic [7:0] y_const,
  output logic [7:0] y_var,
  output logic [7:0] y_logical
);
  assign y_const = a <<< 2;
  assign y_var = a <<< shamt;
  assign y_logical = a << shamt;
endmodule
