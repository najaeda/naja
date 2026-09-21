-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
use std.textio.all;
entity logical_tb is end;
architecture test of logical_tb is
  signal a, b, c, y : bit;
begin
  dut: entity work.logic_nested port map(a, b, c, y);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    for stimulus in 0 to 7 loop
      if (stimulus / 4) mod 2 = 1 then a <= '1'; else a <= '0'; end if;
      if (stimulus / 2) mod 2 = 1 then b <= '1'; else b <= '0'; end if;
      if stimulus mod 2 = 1 then c <= '1'; else c <= '0'; end if;
      wait for 1 ns;
      write(row, bit'pos(y)); writeline(trace, row);
    end loop;
    report "LOGICAL_DONE";
    wait;
  end process;
end;
