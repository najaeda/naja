-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity reset_tb is end;
architecture test of reset_tb is
  signal clk, rst, d, q : bit := '0';
begin
  dut: entity work.reset_reg port map(clk, rst, d, q);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    d <= '1'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    clk <= '0'; rst <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    clk <= '0'; rst <= '0'; wait for 1 ns; clk <= '1'; wait for 1 ns;
    write(row, q); writeline(trace, row);
    report "RESET_DONE";
    wait;
  end process;
end;
