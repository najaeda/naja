-- SPDX-License-Identifier: Apache-2.0
-- MOD and REM differ for negative operands

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is

begin

  check: process
  begin
    assert ((-7) mod 3) = 2 and ((-7) rem 3) = -1 and (7 mod (-3)) = -2 severity failure;
    report "PROBE_PASS:mod_rem" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
