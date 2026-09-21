use std.textio.all;
entity pipeline_tb is end;
architecture test of pipeline_tb is
  signal clk, d, q : bit;
begin
  dut: entity work.pipeline port map(clk, d, q);
  process
    constant stimulus : bit_vector := "10110010";
    file trace : text open write_mode is "trace.txt";
    variable row : line;
    variable held : bit;
  begin
    for i in stimulus'range loop
      d <= stimulus(i);
      wait for 1 ns;
      held := q;
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
    report "PIPELINE_DONE";
    wait;
  end process;
end;
