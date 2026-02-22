// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module compatible_net_null_like_top(
  input logic a,
  output logic y
);
  logic not_y;
  assign y = ~a;
endmodule
