// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module top(input [-4:-4] in, output [6:6] out);
wire [5:5] feedtru;

assign feedtru[5] = in[-4];
assign out[6] = feedtru[5];

endmodule //top
