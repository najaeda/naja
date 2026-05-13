// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
// Non-ANSI style: module header lists only port names; direction/type declarations follow in the body.

module non_ansi_ports_top(a, b, y, z);
  input a;
  input [3:0] b;
  output y;
  output [3:0] z;

  assign y = a;
  assign z = b;
endmodule
