-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity vectors_tb is end;
architecture test of vectors_tb is
  signal a : bit_vector(0 to 3);
  signal b : bit_vector(7 downto 4);
  signal sel : bit;
  signal y : bit_vector(3 downto 0);
begin
  dut: entity work.vector_mux port map(a, b, sel, y);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    a <= "1001"; b <= "0110"; sel <= '0'; wait for 1 ns;
    write(row, y); writeline(trace, row);
    sel <= '1'; wait for 1 ns;
    write(row, y); writeline(trace, row);
    a <= "0011"; wait for 1 ns;
    write(row, y); writeline(trace, row);
    report "VECTORS_DONE";
    wait;
  end process;
end;
