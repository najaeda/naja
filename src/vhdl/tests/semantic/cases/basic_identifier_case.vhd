-- SPDX-License-Identifier: Apache-2.0
-- Basic identifiers are case insensitive

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant MiXeD : integer := 7;
begin

  check: process
  begin
    assert mixed = MIXED and mixed = 7 severity failure;
    report "PROBE_PASS:basic_identifier_case" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
