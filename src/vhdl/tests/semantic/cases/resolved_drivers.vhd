-- SPDX-License-Identifier: Apache-2.0
-- Resolved drivers produce non-binary values

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal q : std_logic;
begin
q <= '0';
q <= '1';
  check: process
  begin
    wait for 1 ns; assert q = 'X' severity failure;
    report "PROBE_PASS:resolved_drivers" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
