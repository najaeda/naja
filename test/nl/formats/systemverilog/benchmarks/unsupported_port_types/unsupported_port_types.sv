// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module unsupported_port_types(
  input logic a,
  input shortreal wide_f,
  input string str_i,
  output logic y
);
  assign y = a;
endmodule
