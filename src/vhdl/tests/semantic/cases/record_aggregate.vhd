-- SPDX-License-Identifier: Apache-2.0
-- Records preserve declared field types

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
type payload is record data : bit_vector(3 downto 0); valid : boolean; end record;
constant p : payload := (valid => true, data => "1010");
begin

  check: process
  begin
    assert p.valid and p.data = "1010" severity failure;
    report "PROBE_PASS:record_aggregate" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
