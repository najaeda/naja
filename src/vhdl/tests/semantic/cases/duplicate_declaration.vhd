-- SPDX-License-Identifier: Apache-2.0
-- Duplicate declarations are rejected

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant n : integer := 1;
constant n : integer := 2;
begin

  check: process
  begin
    
    report "PROBE_PASS:duplicate_declaration" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
