-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

entity pipeline is
  port (clk, d : in bit; q : out bit);
end;
architecture rtl of pipeline is
  signal stage : bit;
begin
  process(clk) begin
    if rising_edge(clk) then
      stage <= d;
      q <= stage;
    end if;
  end process;
end;
