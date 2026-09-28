-- SPDX-License-Identifier: Apache-2.0
-- Ascending bounds and attributes

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant a : bit_vector(2 to 5) := "1001";
begin

  check: process
  begin
    assert a'ascending and a'left = 2 and a'right = 5 and a(2) = '1' and a(3) = '0' severity failure;
    report "PROBE_PASS:ascending_array" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
