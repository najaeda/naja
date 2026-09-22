-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity enabled_tb is end;
architecture test of enabled_tb is
  signal clk, en, d, q : bit := '0';
begin
  dut: entity work.enabled_reg port map(clk, en, d, q);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    d <= '1'; en <= '1'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    clk <= '0'; d <= '0'; en <= '0'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    clk <= '0'; en <= '1'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    report "ENABLE_DONE";
    wait;
  end process;
end;
