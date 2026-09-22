-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity enabled_reg is
  port (clk, en, d : in bit; q : out bit);
end;
architecture rtl of enabled_reg is
begin
  process(clk) begin
    if rising_edge(clk) then
      if en = '1' then q <= d; end if;
    end if;
  end process;
end;
