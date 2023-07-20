// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module model(input i, output o, inout io);
endmodule //model

module design(input i0, input [31:0] i1, output o);
wire net_0;
wire [31:0] net_1;
wire n1;
wire n2;

model instance1 (
  .i(),
  .o(n1),
  .io(n2)
);

model instance2 (
  .i(n1),
  .o(),
  .io(n2)
);
endmodule //design
