// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module reg_sv(input logic clk, input logic d, output logic q);
  always_ff @(posedge clk) q <= d;
endmodule
