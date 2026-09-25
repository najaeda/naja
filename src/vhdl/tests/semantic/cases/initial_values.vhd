-- SPDX-License-Identifier: Apache-2.0
-- Explicit initialization and enumeration default

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal explicit_value : bit := '1';
signal default_value : std_logic;
begin

  check: process
  begin
    assert explicit_value = '1' and default_value = 'U' severity failure;
    report "PROBE_PASS:initial_values" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
