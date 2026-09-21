-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity retained_variables is
  port (clk, d : in bit; q : out bit);
end;
architecture rtl of retained_variables is
begin
  process(clk)
    variable retained : bit;
  begin
    if clk'event and clk = '1' then
      q <= retained;
      retained := d;
    end if;
  end process;
end;
