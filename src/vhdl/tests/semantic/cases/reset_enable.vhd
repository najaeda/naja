-- SPDX-License-Identifier: Apache-2.0
-- Asynchronous reset precedes clock enable

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal clk,rst,en,d,q : std_logic := '0';
begin
p: process(clk,rst) begin
  if rst = '1' then q <= '0'; elsif rising_edge(clk) then
    if en = '1' then q <= d; end if;
  end if;
end process;
  check: process
  begin
    d <= '1'; en <= '1'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    assert q = '1' severity failure;
    clk <= '0'; en <= '0'; d <= '0'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    assert q = '1' severity failure;
    rst <= '1'; wait for 1 ns; assert q = '0' severity failure;
    report "PROBE_PASS:reset_enable" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
