-- SPDX-License-Identifier: Apache-2.0
-- Process variables retain state across activations

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal clk : bit := '0';
signal q : integer := 0;
begin
p: process(clk) variable n : integer := 0; begin
  if clk'event and clk = '1' then n := n + 1; q <= n; end if;
end process;
  check: process
  begin
    wait for 1 ns; clk <= '1'; wait for 1 ns; assert q = 1 severity failure;
    clk <= '0'; wait for 1 ns; clk <= '1'; wait for 1 ns; assert q = 2 severity failure;
    report "PROBE_PASS:variable_retention" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
