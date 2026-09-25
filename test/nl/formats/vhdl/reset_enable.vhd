-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity reset_enable_reg is
  port (clk, rst, en, d : in bit; q : out bit);
end;
architecture rtl of reset_enable_reg is
begin
  process(clk) begin
    if rising_edge(clk) then
      if rst = '1' then
        q <= '0';
      elsif en = '1' then
        q <= d;
      end if;
    end if;
  end process;
end;
