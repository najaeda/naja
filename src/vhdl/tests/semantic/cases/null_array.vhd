-- SPDX-License-Identifier: Apache-2.0
-- Null ranges have zero elements

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant a : bit_vector(1 to 0) := "";
begin

  check: process
  begin
    assert a'length = 0 severity failure;
    for i in a'range loop assert false severity failure; end loop;
    report "PROBE_PASS:null_array" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
