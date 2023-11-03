/* 
  Copyright The Naja Authors.
  SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>

  SPDX-License-Identifier: Apache-2.0
  
  Error when instance model does not exist
*/

module test(input i, output o, inout io);
  unknown inst();
endmodule