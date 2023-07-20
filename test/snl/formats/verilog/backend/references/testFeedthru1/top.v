// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module top(input in, output out);
wire [5:5] feedtru;

assign feedtru[5] = in;
assign out = feedtru[5];

endmodule //top
