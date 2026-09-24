-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;

entity async_reset_test is
  port(clk, rstn, rst, enable : in std_ulogic;
       d : in std_ulogic_vector(3 downto 0);
       q : out std_ulogic_vector(3 downto 0);
       p : out std_ulogic_vector(0 to 3);
       held : out std_ulogic);
end;

architecture rtl of async_reset_test is
  signal low_state : std_ulogic_vector(3 downto 0) := "1100";
  signal high_state : std_ulogic_vector(0 to 3) := "0101";
  signal retained : std_ulogic := '0';
begin
  process(rstn, clk) begin
    if (rstn = '0') then
      low_state <= (others => '0');
      low_state(1 downto 0) <= "11";
    elsif rising_edge(clk) then
      if enable = '1' then low_state <= d; end if;
    end if;
  end process;

  process(clk, rst) begin
    if '1' = rst then
      for i in 0 to 1 loop high_state(i) <= '1'; end loop;
      high_state(2 to 3) <= "00";
    elsif clk'event and clk = '1' then
      high_state <= d;
      retained <= not retained;
    end if;
  end process;
  q <= low_state;
  p <= high_state;
  held <= retained;
end;
