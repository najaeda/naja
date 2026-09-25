-- SPDX-License-Identifier: Apache-2.0
-- Package body completes deferred constant
package values is constant answer : integer; end package;
package body values is constant answer : integer := 42; end package body;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant n : integer := work.values.answer;
begin

  check: process
  begin
    assert n = 42 severity failure;
    report "PROBE_PASS:deferred_constant" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
