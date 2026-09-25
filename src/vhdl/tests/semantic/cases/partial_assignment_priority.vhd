-- SPDX-License-Identifier: Apache-2.0
-- Later signal assignment overrides overlapping bits

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal clk : bit := '0';
signal q : bit_vector(3 downto 0) := "0000";
begin
p: process(clk) begin
  if clk'event and clk = '1' then q <= "1111"; q(2 downto 1) <= "01"; end if;
end process;
  check: process
  begin
    wait for 1 ns; clk <= '1'; wait for 1 ns; assert q = "1011" severity failure;
    report "PROBE_PASS:partial_assignment_priority" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
