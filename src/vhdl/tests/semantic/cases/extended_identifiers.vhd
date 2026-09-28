-- SPDX-License-Identifier: Apache-2.0
-- Extended identifiers preserve case

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity probe is end entity;
architecture test of probe is
constant \Case\ : integer := 3;
constant \case\ : integer := 8;
begin

  check: process
  begin
    assert \Case\ = 3 and \case\ = 8 severity failure;
    report "PROBE_PASS:extended_identifiers" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
