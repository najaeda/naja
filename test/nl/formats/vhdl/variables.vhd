-- SPDX-License-Identifier: Apache-2.0
entity variables is
  port (clk, d : in bit; delayed, immediate, captured : out bit);
end;
architecture rtl of variables is
  signal stage : bit;
begin
  process(clk)
    variable temp, copy : bit;
  begin
    if clk'event and clk = '1' then
      stage <= d;
      temp := stage;
      copy := temp;
      delayed <= copy;
      temp := d;
      immediate <= temp;
      captured <= copy;
      copy := d;
    end if;
  end process;
end;
