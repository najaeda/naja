// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

module always_comb_ignored(
  input logic a,
  input logic b,
  output logic y
);
  always_comb begin
    y = a & b;
  end
endmodule
