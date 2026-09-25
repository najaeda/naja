-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity logic_nested is
  port (a, b, c : in bit; y : out bit);
end;
architecture rtl of logic_nested is
begin
  y <= (a and b) xor not c;
end;
