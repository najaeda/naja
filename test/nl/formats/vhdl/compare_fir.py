# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare a generated FIR's canonical netlist with NVC cycle outputs."""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def run(command, cwd, env=None):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=120)
    print(result.stdout, end="")
    result.check_returncode()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nvc", required=True)
    parser.add_argument("--adapter", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--top", required=True)
    parser.add_argument("--lanes", type=int, required=True)
    parser.add_argument("--width", type=int, required=True)
    parser.add_argument("--test", required=True)
    args = parser.parse_args()
    source = args.source.resolve()
    adapter = str(Path(args.adapter).resolve())
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-fir-") as work:
        mask = (1 << args.lanes) - 1
        patterns = [0 if cycle < 4 else 1 << (cycle - 4) if cycle < 4 + args.lanes
                    else ((cycle * 1777 + 91) ^ (0 if cycle % 5 else mask)) & mask
                    for cycle in range(128)]
        Path(work, "inputs.txt").write_text("".join(f"{value}\n" for value in patterns))
        Path(work, "tb.vhd").write_text(f"""
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
entity fir_tb is end;
architecture test of fir_tb is
  signal ck : std_logic := '0';
  signal d : std_logic_vector({args.lanes - 1} downto 0) := (others => '0');
  signal q : std_logic_vector({args.width - 1} downto 0);
begin
  dut: entity work.{args.top} port map(ck => ck, regx_in => d, y_out => q);
  process
    file inputs : text open read_mode is "inputs.txt";
    file outputs : text open write_mode is "trace.txt";
    variable row : line;
    variable pattern : natural;
  begin
    for cycle in 0 to 127 loop
      readline(inputs, row); read(row, pattern);
      d <= std_logic_vector(to_unsigned(pattern, d'length));
      ck <= '0'; wait for 5 ns;
      ck <= '1'; wait for 5 ns;
      if cycle >= 3 then
        write(row, to_integer(unsigned(q))); writeline(outputs, row);
      end if;
    end loop;
    wait;
  end process;
end;
""")
        run([args.nvc, "--std=2008", "-a", str(source), "tb.vhd"], work)
        run([args.nvc, "--std=2008", "-e", "fir_tb"], work)
        run([args.nvc, "--std=2008", "-r", "fir_tb", "--stop-time=1280ns"], work)
        trace = Path(work, "trace.txt")
        if len(trace.read_text().splitlines()) != 125:
            raise RuntimeError("incomplete FIR reference trace")
        run([adapter, f"--gtest_filter={args.test}"], work,
            dict(os.environ, VHDL_FIR_REFERENCE=str(trace), VHDL_FIR16_BENCHMARK=str(source)))
    print("All 125 FIR outputs after pipeline initialization match NVC.")


if __name__ == "__main__":
    main()
