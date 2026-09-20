-- SPDX-License-Identifier: Apache-2.0
-- Function arguments preserve actual bounds

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
function left_index(a : bit_vector) return integer is begin return a'left; end;
constant a : bit_vector(4 to 6) := "101";
begin

  check: process
  begin
    assert left_index(a) = 4 severity failure;
    report "PROBE_PASS:unconstrained_function" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
