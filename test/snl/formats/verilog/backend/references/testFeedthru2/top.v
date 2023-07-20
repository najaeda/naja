// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module top(input [-4:-4] in, output [6:6] out);
wire feedtru;

assign feedtru = in[-4];
assign out[6] = feedtru;

endmodule //top
