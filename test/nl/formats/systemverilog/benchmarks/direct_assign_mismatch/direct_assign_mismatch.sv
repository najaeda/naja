// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module direct_assign_mismatch(
  input logic in_s,
  input logic [1:0] in_b2,
  input logic [2:0] in_b3,
  output logic out_s,
  output logic [1:0] out_b2
);
  // Scalar <- bus (unsupported direct assign shape in current builder)
  assign out_s = in_b2;
  // Bus <- scalar (unsupported direct assign shape in current builder)
  assign out_b2 = in_s;
  // Bus <- bus with width mismatch (unsupported direct assign shape)
  assign out_b2 = in_b3;
endmodule
