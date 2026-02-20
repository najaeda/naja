// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module child(input logic a, output logic y);
  assign y = a;
endmodule

module model_reuse_top(
  input logic a0,
  input logic a1,
  output logic y0,
  output logic y1
);
  child u0(.a(a0), .y(y0));
  child u1(.a(a1), .y(y1));
endmodule
