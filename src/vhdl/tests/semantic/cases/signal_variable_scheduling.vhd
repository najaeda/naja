-- SPDX-License-Identifier: Apache-2.0
-- Signal reads old value; variable reads immediate update

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal clk : bit := '0';
signal d,s,qs,qv : bit := '0';
begin
p: process(clk)
  variable v : bit := '0';
  begin
    if clk'event and clk = '1' then s <= d; qs <= s; v := d; qv <= v; end if;
  end process;
  check: process
  begin
    d <= '1'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    assert s = '1' and qs = '0' and qv = '1' severity failure;
    clk <= '0'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    assert qs = '1' severity failure;
    report "PROBE_PASS:signal_variable_scheduling" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
