-- SPDX-License-Identifier: Apache-2.0
-- PROCESS(ALL) reacts to both read signals

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal a,b,q : bit := '0';
begin
p: process(all) begin q <= a xor b; end process;
  check: process
  begin
    wait for 1 ns; a <= '1'; wait for 1 ns; assert q = '1' severity failure;
    b <= '1'; wait for 1 ns; assert q = '0' severity failure;
    report "PROBE_PASS:process_all" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
