// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module vector_mux_sv(
  input logic [0:3] a,
  input logic [7:4] b,
  input logic sel,
  output logic [3:0] y
);
  assign y = sel ? a : b;
endmodule
