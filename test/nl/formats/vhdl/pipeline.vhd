entity pipeline is
  port (clk, d : in bit; q : out bit);
end;
architecture rtl of pipeline is
  signal stage : bit;
begin
  process(clk) begin
    if clk'event and clk = '1' then
      stage <= d;
      q <= stage;
    end if;
  end process;
end;
