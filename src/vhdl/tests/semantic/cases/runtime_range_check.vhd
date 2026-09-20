-- SPDX-License-Identifier: Apache-2.0
-- Dynamic subtype bounds are checked

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
subtype small is integer range 0 to 3;
signal input_value : integer := 5;
signal output_value : small;
begin

  check: process
  begin
    wait for 1 ns; output_value <= input_value; wait for 1 ns;
    report "PROBE_PASS:runtime_range_check" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
