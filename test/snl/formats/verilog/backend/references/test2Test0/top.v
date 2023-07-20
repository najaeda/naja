// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module top();
wire [-2:2] bus;
wire scalar;

assign bus[-2] = 1'b0;

assign bus[-1] = scalar;

assign bus[0] = 1'b1;

assign bus[1] = scalar;

assign bus[2] = 1'b0;
endmodule //top
