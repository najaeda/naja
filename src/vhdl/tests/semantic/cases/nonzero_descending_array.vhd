-- SPDX-License-Identifier: Apache-2.0
-- Nonzero descending bounds

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant a : bit_vector(9 downto 6) := "1010";
begin

  check: process
  begin
    assert not a'ascending and a'low = 6 and a'high = 9 and a(9) = '1' and a(6) = '0' severity failure;
    report "PROBE_PASS:nonzero_descending_array" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
