-- SPDX-License-Identifier: Apache-2.0
-- Assignment associates elements by position

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal a : bit_vector(2 to 5) := "1010";
signal b : bit_vector(9 downto 6);
begin
b <= a;
  check: process
  begin
    wait for 1 ns; assert b = "1010" and b(9) = a(2) and b(6) = a(5) severity failure;
    report "PROBE_PASS:positional_array_assignment" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
