-- SPDX-License-Identifier: Apache-2.0
-- Unary sign applies after multiplying operators, including MOD and REM.
entity probe is end entity;
architecture test of probe is
begin
  check: process
  begin
    assert (-7 mod 3) = -1 and ((-7) mod 3) = 2 severity failure;
    report "PROBE_PASS:unary_precedence" severity note;
    std.env.stop;
    wait;
  end process;
end architecture;
