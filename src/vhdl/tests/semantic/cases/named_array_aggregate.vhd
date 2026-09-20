-- SPDX-License-Identifier: Apache-2.0
-- Named choices use source indices

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant a : bit_vector(7 downto 4) := (7 => '1', 5 => '1', others => '0');
begin

  check: process
  begin
    assert a = "1010" severity failure;
    report "PROBE_PASS:named_array_aggregate" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
