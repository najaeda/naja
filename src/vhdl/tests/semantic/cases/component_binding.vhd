-- SPDX-License-Identifier: Apache-2.0
-- Configuration specification chooses architecture
entity cell is port(q : out bit); end;
architecture one of cell is begin q <= '1'; end;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
component cell is port(q : out bit); end component;
for u : cell use entity work.cell(one);
signal q : bit;
begin
u: cell port map(q);
  check: process
  begin
    wait for 1 ns; assert q = '1' severity failure;
    report "PROBE_PASS:component_binding" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
