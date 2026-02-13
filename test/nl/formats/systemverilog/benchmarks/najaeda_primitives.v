// SPDX-FileCopyrightText: 2026 The Naja authors
//
// SPDX-License-Identifier: Apache-2.0

// Primitive behavioral shims used to simulate najaeda-generated Verilog.
module fa(
  input wire A,
  input wire B,
  input wire CI,
  output wire S,
  output wire CO
);
  assign {CO, S} = A + B + CI;
endmodule

module mux2(
  input wire A,
  input wire B,
  input wire S,
  output wire Y
);
  assign Y = S ? B : A;
endmodule

module dff(
  input wire C,
  input wire D,
  output reg Q
);
  always @(posedge C) begin
    Q <= D;
  end
endmodule
