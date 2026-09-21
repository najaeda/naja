-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity variables_tb is end;
architecture test of variables_tb is
  signal clk, d, delayed, immediate, captured : bit;
begin
  dut: entity work.variables port map(clk, d, delayed, immediate, captured);
  process
    constant stimulus : bit_vector := "10110010";
    file trace : text open write_mode is "trace.txt";
    variable row : line;
    variable held : bit_vector(0 to 2);
  begin
    for i in stimulus'range loop
      d <= stimulus(i);
      wait for 1 ns;
      clk <= '1';
      wait for 1 ns;
      assert immediate = stimulus(i) severity failure;
      if i > stimulus'left then
        assert delayed = stimulus(i - 1) severity failure;
        assert captured = delayed severity failure;
        write(row, bit'pos(delayed)); write(row, string'(" "));
        write(row, bit'pos(immediate)); write(row, string'(" "));
        write(row, bit'pos(captured)); writeline(trace, row);
      end if;
      held := delayed & immediate & captured;
      d <= not stimulus(i);
      wait for 1 ns;
      assert (delayed & immediate & captured) = held severity failure;
      clk <= '0';
      wait for 1 ns;
      assert (delayed & immediate & captured) = held severity failure;
    end loop;
    report "VARIABLES_DONE";
    wait;
  end process;
end;
