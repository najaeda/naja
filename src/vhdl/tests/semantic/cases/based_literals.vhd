-- SPDX-License-Identifier: Apache-2.0
-- Based literals and bit-string literals

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant value : integer := 16#F_F#;
constant bits : bit_vector := X"A5";
begin

  check: process
  begin
    assert value = 255 and bits = "10100101" severity failure;
    report "PROBE_PASS:based_literals" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
