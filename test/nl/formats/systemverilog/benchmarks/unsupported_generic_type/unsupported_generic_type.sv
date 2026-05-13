// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module unsupported_generic_type(
  input logic [3:0] arr [0:1],
  output logic y
);
  assign y = 1'b0;
endmodule
