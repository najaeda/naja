-- SPDX-License-Identifier: Apache-2.0
-- Explicit architecture binding
entity cell is port(q : out bit); end;
architecture zero of cell is begin q <= '0'; end;
architecture one of cell is begin q <= '1'; end;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal a,b : bit;
begin
a_inst: entity work.cell(zero) port map(a);
b_inst: entity work.cell(one) port map(b);
  check: process
  begin
    wait for 1 ns; assert a = '0' and b = '1' severity failure;
    report "PROBE_PASS:architecture_selection" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
