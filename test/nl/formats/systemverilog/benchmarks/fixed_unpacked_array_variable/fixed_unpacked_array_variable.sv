// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module fixed_unpacked_array_variable_top(
  input logic [31:0] in0,
  input logic [31:0] in1,
  output logic [31:0] y
);
  logic [31:0] fetch_instructions [1:0];

  assign y = in0;
endmodule
