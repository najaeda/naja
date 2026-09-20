-- SPDX-License-Identifier: Apache-2.0
-- Expected type resolves return-type overloads

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
function pick return integer is begin return 7; end;
function pick return boolean is begin return true; end;
constant n : integer := pick;
constant b : boolean := pick;
begin

  check: process
  begin
    assert n = 7 and b severity failure;
    report "PROBE_PASS:return_type_overload" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
