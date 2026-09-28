-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
library ieee;
use ieee.std_logic_1164.all;
entity memory_test is
  port(clk, ce, we, reset : in std_logic;
       d : in std_logic_vector(7 downto 0);
       q, async_q : out std_logic_vector(7 downto 0));
end;
architecture rtl of memory_test is
  type words is array (0 to 3) of std_logic_vector(7 downto 0);
  signal ram : words;
  signal wr_addr, rd_addr : integer range 0 to 3;
begin
  async_q <= ram(rd_addr);
  process(clk) begin
    if rising_edge(clk) then
      if reset = '1' then
        wr_addr <= 0;
        rd_addr <= 0;
      elsif ce = '1' then
        if we = '1' then
          if wr_addr = 3 then wr_addr <= 0; else wr_addr <= wr_addr + 1; end if;
        end if;
        if rd_addr = 3 then rd_addr <= 0; else rd_addr <= rd_addr + 1; end if;
      end if;
      if ce = '1' then
        if we = '1' then ram(wr_addr) <= d; end if;
        q <= ram(rd_addr);
      end if;
    end if;
  end process;
end;
