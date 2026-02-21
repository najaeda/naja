// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module seq_list_single_empty_stmt_unsupported(
  input logic clk,
  output logic q
);
  // Single-item statement list with an empty statement.
  always begin
    ;
  end
endmodule
