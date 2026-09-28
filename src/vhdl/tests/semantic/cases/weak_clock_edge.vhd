-- SPDX-License-Identifier: Apache-2.0
-- RISING_EDGE recognizes weak low-to-high transition

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal clk : std_logic := 'L';
signal count : integer := 0;
begin
p: process(clk) begin if rising_edge(clk) then count <= count + 1; end if; end process;
  check: process
  begin
    wait for 1 ns; clk <= 'H'; wait for 1 ns; assert count = 1 severity failure;
    report "PROBE_PASS:weak_clock_edge" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
