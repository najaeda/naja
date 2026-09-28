-- SPDX-License-Identifier: Apache-2.0
-- Operators are resolved declarations

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
type word is (low, high);
function "+" (a,b : word) return word is begin return high; end;
constant result : word := low + low;
begin

  check: process
  begin
    assert result = high severity failure;
    report "PROBE_PASS:operator_overload" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
