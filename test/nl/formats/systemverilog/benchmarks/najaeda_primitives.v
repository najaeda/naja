// SPDX-FileCopyrightText: 2026 The Naja authors
//
// SPDX-License-Identifier: Apache-2.0

// Primitive behavioral shims used to simulate najaeda-generated Verilog.
// Canonical module names are naja_* to match NLDB0 primitive names.

module naja_fa(
  input wire A,
  input wire B,
  input wire CI,
  output wire S,
  output wire CO
);
  assign {CO, S} = A + B + CI;
endmodule

module naja_mux2(
  input wire A,
  input wire B,
  input wire S,
  output wire Y
);
  assign Y = S ? B : A;
endmodule

module naja_dff(
  input wire C,
  input wire D,
  output reg Q
);
  always @(posedge C) begin
    Q <= D;
  end
endmodule

module naja_dffrn(
  input wire C,
  input wire D,
  input wire RN,
  output reg Q
);
  always @(posedge C or negedge RN) begin
    if (!RN) Q <= 1'b0;
    else Q <= D;
  end
endmodule

module naja_dffe(
  input wire C,
  input wire D,
  input wire E,
  output reg Q
);
  always @(posedge C) begin
    if (E) Q <= D;
  end
endmodule

module naja_dffre(
  input wire C,
  input wire D,
  input wire E,
  input wire R,
  output reg Q
);
  always @(posedge C or posedge R) begin
    if (R) Q <= 1'b0;
    else if (E) Q <= D;
  end
endmodule

module naja_dffse(
  input wire C,
  input wire D,
  input wire E,
  input wire S,
  output reg Q
);
  always @(posedge C or posedge S) begin
    if (S) Q <= 1'b1;
    else if (E) Q <= D;
  end
endmodule
