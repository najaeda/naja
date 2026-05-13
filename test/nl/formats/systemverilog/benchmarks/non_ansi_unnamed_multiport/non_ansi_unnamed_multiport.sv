// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
// Non-ANSI port concatenation in the module header creates an unnamed multi-port symbol.

module non_ansi_unnamed_multiport_top({a, b}, y);
  input a;
  input b;
  output y;

  assign y = a & b;
endmodule
