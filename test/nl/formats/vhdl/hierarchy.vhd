-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity inv4 is
  port (a : in bit_vector(0 to 3); y : out bit_vector(7 downto 4));
end;
architecture rtl of inv4 is
begin
  y <= not a;
end;

entity hierarchy_top is
  port (a : in bit_vector(3 downto 0); y : out bit_vector(0 to 3));
end;
architecture structural of hierarchy_top is
  signal mid : bit_vector(11 downto 8);
begin
  u0: entity work.inv4 port map(a, mid);
  u1: entity work.inv4 port map(mid, y);
end;
