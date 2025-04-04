/* 
  Copyright The Naja Authors.
  SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>

  SPDX-License-Identifier: Apache-2.0
  
  Error: referencing bit slice on scalar net.
*/

module model(input[2:0] i);
endmodule

module test();
  wire net0;

  model inst(.i(net0[2:0]));
endmodule