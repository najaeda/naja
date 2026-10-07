// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module add4(input [3:0] a, b, output [3:0] s);
  assign s = a + b;
endmodule

module sub4(input [3:0] a, b, output [3:0] s);
  assign s = a - b;
endmodule

module mul4(input [3:0] a, b, output [7:0] s);
  assign s = a * b;
endmodule

module mulconst4(input [3:0] a, output [7:0] s);
  assign s = a * 8'd5;
endmodule
