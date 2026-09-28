-- SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
-- SPDX-License-Identifier: Apache-2.0
entity vector_mux is
  port (
    a : in bit_vector(0 to 3);
    b : in bit_vector(7 downto 4);
    sel : in bit;
    y : out bit_vector(3 downto 0));
end;
architecture rtl of vector_mux is
begin
  y <= a when sel = '1' else b;
end;
