-- SPDX-License-Identifier: Apache-2.0
-- Unconstrained overload candidates are rejected

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
function f return integer is begin return 1; end;
function f return boolean is begin return true; end;
constant n : boolean := f = f;
begin

  check: process
  begin
    
    report "PROBE_PASS:ambiguous_overload" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
