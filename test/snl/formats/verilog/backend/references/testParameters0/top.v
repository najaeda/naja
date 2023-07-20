// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0
//

module model();
parameter PARAM0 = 8 ;
parameter PARAM1 = "FALSE" ;
parameter PARAM2 = "TRUE" ;
parameter PARAM3 = 4'hF ;
parameter PARAM4 = 4'b0011 ;
parameter PARAM5 = "HELLO" ;

endmodule //model

module top();
model #(
  .PARAM1("TRUE"),
  .PARAM2("FALSE")
) ins (
);
endmodule //top
