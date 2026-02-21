// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module child_scalar(
  input logic a,
  output logic y
);
  assign y = a;
endmodule

module child_bus2(
  input logic [1:0] a,
  output logic [1:0] y
);
  assign y = a;
endmodule

module instance_connection_edge_cases(
  input logic s,
  output logic y,
  output logic [1:0] yb
);
  // Null expression for named port connection.
  child_scalar u_null(.a(), .y(y));
  // Unresolved concat expression for scalar port connection.
  child_scalar u_concat(.a({s, s}), .y());
  // Scalar net connected to bus port (bus/scalar mismatch branch).
  child_bus2 u_bus(.a(s), .y(yb));
  // Positional connection form.
  child_scalar u_positional(s, y);
endmodule
