-- SPDX-License-Identifier: Apache-2.0
-- Distinct array types need explicit conversion

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
type first is array(natural range <>) of bit;
type second is array(natural range <>) of bit;
constant a : first(1 downto 0) := "10";
constant b : second(1 downto 0) := a;
begin

  check: process
  begin
    
    report "PROBE_PASS:distinct_array_types" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
