-- SPDX-License-Identifier: Apache-2.0
-- WORK denotes the current library
package values is constant answer : integer := 11; end package;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant n : integer := work.values.answer;
begin

  check: process
  begin
    assert n = 11 severity failure;
    report "PROBE_PASS:work_library" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
