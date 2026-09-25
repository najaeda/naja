-- SPDX-License-Identifier: Apache-2.0
-- Signed extension differs from unsigned extension

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant a : signed(3 downto 0) := "1101";
begin

  check: process
  begin
    assert resize(a, 8) = "11111101" and to_integer(a) = -3 severity failure;
    report "PROBE_PASS:signed_resize" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
