# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare inferred RAM cycles and optional Verilog export with NVC."""
import argparse
import os
from pathlib import Path
import tempfile

from compare_pipeline import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nvc", required=True)
    parser.add_argument("--adapter", required=True)
    parser.add_argument("--iverilog")
    parser.add_argument("--vvp")
    args = parser.parse_args()
    adapter = str(Path(args.adapter).resolve())
    source = Path(__file__).resolve().with_name("memory.vhd")
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-memory-") as directory:
        work = Path(directory)
        work.joinpath("tb.vhd").write_text("""
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.textio.all;
entity memory_tb is end;
architecture test of memory_tb is
  signal clk, ce, we, reset : std_logic := '0';
  signal d, q, async_q : std_logic_vector(7 downto 0);
begin
  dut: entity work.memory_test port map(clk, ce, we, reset, d, q, async_q);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
    for cycle in 0 to 79 loop
      if cycle = 0 or cycle = 17 or cycle = 39 then reset <= '1'; else reset <= '0'; end if;
      if cycle > 0 and (cycle < 5 or cycle mod 5 /= 0) then ce <= '1'; else ce <= '0'; end if;
      if cycle < 5 or cycle mod 3 /= 0 then we <= '1'; else we <= '0'; end if;
      d <= std_logic_vector(to_unsigned((cycle * 37 + 11) mod 256, 8));
      clk <= '0'; wait for 5 ns;
      clk <= '1'; wait for 5 ns;
      if cycle >= 6 then
        write(row, to_integer(unsigned(q))); write(row, string'(" "));
        write(row, to_integer(unsigned(async_q))); writeline(trace, row);
      end if;
    end loop;
    wait;
  end process;
end;
""")
        run([args.nvc, "--std=2008", "-a", str(source), "tb.vhd"], work)
        run([args.nvc, "--std=2008", "-e", "memory_tb"], work)
        run([args.nvc, "--std=2008", "-r", "memory_tb", "--stop-time=800ns"], work)
        trace = work / "trace.txt"
        expected = trace.read_text().split()
        if len(expected) != 148:
            raise RuntimeError("incomplete RAM reference trace")
        run([adapter, "--gtest_filter=VHDLConstructorTest.InferredMemoryReadBeforeWriteAndEnableCycles"],
            work, dict(os.environ, VHDL_MEMORY_REFERENCE=str(trace), VHDL_MEMORY_DUMP=str(work)))
        if args.iverilog and args.vvp:
            work.joinpath("tb.v").write_text("""
module memory_tb;
  reg clk = 0, ce = 0, we = 0, reset = 0;
  reg [7:0] d;
  wire [7:0] q, async_q;
  memory_test dut(.clk(clk), .ce(ce), .we(we), .reset(reset), .d(d), .q(q), .async_q(async_q));
  integer cycle, trace;
  initial begin
    trace = $fopen("dump_trace.txt", "w");
    for (cycle = 0; cycle < 80; cycle = cycle + 1) begin
      reset = cycle == 0 || cycle == 17 || cycle == 39;
      ce = cycle > 0 && (cycle < 5 || cycle % 5 != 0);
      we = cycle < 5 || cycle % 3 != 0;
      d = (cycle * 37 + 11) % 256;
      clk = 0; #5; clk = 1; #5;
      if (cycle >= 6) $fdisplay(trace, "%0d %0d", q, async_q);
    end
    $fclose(trace);
    $finish;
  end
endmodule
""")
            run([args.iverilog, "-g2012", "-s", "memory_tb", "-o", "sim", "memory_test.v", "naja_primitives.v", "tb.v"], work)
            run([args.vvp, "sim"], work)
            if work.joinpath("dump_trace.txt").read_text().split() != expected:
                raise RuntimeError("exported RAM Verilog disagrees with NVC")
    print("RAM cycles match NVC, including collisions, enables and address reset.")


if __name__ == "__main__":
    main()
