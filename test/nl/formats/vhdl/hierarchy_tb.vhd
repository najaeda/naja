-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity hierarchy_tb is end;
architecture test of hierarchy_tb is
  signal a : bit_vector(3 downto 0);
  signal y : bit_vector(0 to 3);
begin
  dut: entity work.hierarchy_top port map(a, y);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    a <= "1001"; wait for 1 ns;
    write(row, y); writeline(trace, row);
    a <= "0110"; wait for 1 ns;
    write(row, y); writeline(trace, row);
    a <= "0011"; wait for 1 ns;
    write(row, y); writeline(trace, row);
    report "HIERARCHY_DONE";
    wait;
  end process;
end;
