// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module dynamic_unpacked_variables_ignored_top(
  input logic a,
  output logic y
);
  logic [31:0] decode_queue[$];
  logic [31:0] issue_queue[];
  logic [31:0] scoreboard_queue[int];

  assign y = a;
endmodule
