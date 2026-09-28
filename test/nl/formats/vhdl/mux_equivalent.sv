// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

module mux_sv(input logic a, input logic b, input logic sel, output logic y);
  assign y = sel ? a : b;
endmodule
