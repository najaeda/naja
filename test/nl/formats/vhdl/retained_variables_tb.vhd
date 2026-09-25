-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity retained_variables_tb is end;
architecture test of retained_variables_tb is
  signal clk, d, q : bit;
begin
  dut: entity work.retained_variables port map(clk, d, q);
  process
    constant stimulus : bit_vector := "10110010";
    file trace : text open write_mode is "trace.txt";
    variable row : line;
    variable held : bit;
  begin
    for i in stimulus'range loop
      d <= stimulus(i);
      wait for 1 ns;
      clk <= '1';
      wait for 1 ns;
      if i > stimulus'left then
        assert q = stimulus(i - 1) severity failure;
        write(row, bit'pos(q)); writeline(trace, row);
      end if;
      held := q;
      d <= not stimulus(i);
      wait for 1 ns;
      assert q = held severity failure;
      clk <= '0';
      wait for 1 ns;
      assert q = held severity failure;
    end loop;
    report "RETAINED_VARIABLES_DONE";
    wait;
  end process;
end;
