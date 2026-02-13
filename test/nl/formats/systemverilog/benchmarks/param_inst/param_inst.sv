// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module leaf #(
  parameter int W = 1
) (
  input logic [W-1:0] a,
  output logic [W-1:0] y
);
  assign y = a;
endmodule

module top;
  logic [3:0] a;
  logic [3:0] y;
  leaf #(.W(4)) u0 (.a(a), .y(y));
endmodule
