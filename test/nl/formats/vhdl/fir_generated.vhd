-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
entity tap is
  generic(c1, c2, c3, n : integer);
  port(ck, reg_in : in std_logic; y_out : out std_logic_vector(n-1 downto 0));
end;
architecture rtl of tap is
  signal a, b, c : std_logic;
  signal ma, mb, mc : std_logic_vector(n-1 downto 0);
begin
  process(ck) begin
    if (ck'event and ck = '1') then a <= reg_in; b <= a; c <= b; end if;
  end process;
  ma <= (others => a); mb <= (others => b); mc <= (others => c);
  y_out <= (ma and std_logic_vector(to_unsigned(c1, n))) +
           (mb and std_logic_vector(to_unsigned(c2, n))) +
           (mc and std_logic_vector(to_unsigned(c3, n)));
end;

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_signed.all;
entity fir_generated is
  port(ck : in std_logic; regx_in : in std_logic_vector(3 downto 0);
       y_out : out std_logic_vector(4 downto 0));
end;
architecture rtl of fir_generated is
  constant width : positive := 5;
  type coefficients is array(natural range <>) of integer;
  constant first : coefficients := (1, 3, 6, 10);
  constant second : coefficients(3 downto 0) := (1, 3, 6, 10);
  type positive_indices is array(positive range <>) of integer;
  constant third : positive_indices := (5, 7, 9, 11);
  type results is array(-1 to 2) of std_logic_vector(width-1 downto 0);
  signal result : results;
  component tap
    generic(c1, c2, c3, n : integer);
    port(ck, reg_in : in std_logic; y_out : out std_logic_vector(n-1 downto 0));
  end component;
begin
  lanes: for i in 3 downto 0 generate
  begin
    singleton: for j in 0 to 0 generate
      f: tap generic map(first(i), second(i), third(i+1), width)
        port map(y_out => result(i-1), ck => ck, reg_in => regx_in(i+j));
    end generate singleton;
  end generate lanes;
  y_out <= result(-1) + result(0) + result(1) + result(2);
end;
