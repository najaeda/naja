-- SPDX-License-Identifier: Apache-2.0
-- Array length mismatch is not implicit truncation

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal a : bit_vector(3 downto 0) := "1010";
signal b : bit_vector(1 downto 0);
begin

  check: process
  begin
    wait for 1 ns; b <= a; wait for 1 ns;
    report "PROBE_PASS:runtime_length_check" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
