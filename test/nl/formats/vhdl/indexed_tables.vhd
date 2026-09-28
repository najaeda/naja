-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity indexed_tables is
  port(clk : in std_logic;
       addr : in std_logic_vector(2 downto 0);
       narrow : in std_logic_vector(0 downto 0);
       d : in std_logic_vector(7 downto 0);
       direct_q, signal_q, duplicate_q, late_q, writable_q,
       offset_q, descending_q, narrow_q : out std_logic_vector(7 downto 0));
end;

architecture rtl of indexed_tables is
  type table_type is array (0 to 2) of std_logic_vector(7 downto 0);
  type offset_type is array (3 to 5) of std_logic_vector(7 downto 0);
  type descending_type is array (5 downto 3) of std_logic_vector(0 to 7);
  constant rom : table_type := (x"52", x"09", x"A6");
  constant offset_rom : offset_type := (x"52", x"09", x"A6");
  constant descending_rom : descending_type := (x"A6", x"09", x"52");
  signal rom_signal, late_signal, writable : table_type;
begin
  rom_signal <= rom;
  direct_q <= rom(to_integer(unsigned(addr)));
  signal_q <= rom_signal(to_integer(unsigned(addr)));
  duplicate_q <= rom_signal(to_integer(unsigned(addr)));
  late_q <= late_signal(to_integer(unsigned(addr)));
  late_signal <= rom;
  writable_q <= writable(to_integer(unsigned(addr)));
  offset_q <= offset_rom(to_integer(unsigned(addr)));
  descending_q <= descending_rom(to_integer(unsigned(addr)));
  narrow_q <= rom(to_integer(unsigned(narrow)));
  process(clk) begin
    if rising_edge(clk) then
      writable(0) <= d;
      writable(1) <= not d;
      writable(2) <= d xor x"A5";
    end if;
  end process;
end;
