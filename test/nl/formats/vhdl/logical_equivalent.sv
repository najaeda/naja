// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module logic_nested_sv(input logic a, b, c, output logic y);
  assign y = (a & b) ^ ~c;
endmodule
