// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module continuous_unbased_unsized_literals_unsupported_top(
  input logic [3:0] a,
  input logic [3:0] b,
  output logic [3:0] y_known,
  output logic [3:0] y_unknown
);
  // Known unbased-unsized literal should expand to a constant vector.
  assign y_known = a + '1;
  // Unknown unbased-unsized literal should fail constant expansion.
  assign y_unknown = b + 'x;
endmodule
