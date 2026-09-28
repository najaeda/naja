-- SPDX-License-Identifier: Apache-2.0
-- Generic specialization and generate expansion
entity ones is generic (n : positive); port(q : out bit_vector(n-1 downto 0)); end;
architecture rtl of ones is begin
  gen: for i in 0 to n-1 generate q(i) <= '1'; end generate;
end;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
signal a : bit_vector(2 downto 0);
signal b : bit_vector(4 downto 0);
begin
a_inst: entity work.ones(rtl) generic map (n => 3) port map (a);
b_inst: entity work.ones(rtl) generic map (n => 5) port map (b);
  check: process
  begin
    wait for 1 ns; assert a = "111" and b = "11111" severity failure;
    report "PROBE_PASS:generic_generate" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
