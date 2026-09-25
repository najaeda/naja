# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare reset transitions between clocks with NVC and exported Verilog."""
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
    source = Path(__file__).with_name("asynchronous_reset.vhd").resolve()
    events = [
        (0, 1, 0, 0, 5), (0, 0, 0, 0, 5), (0, 1, 0, 0, 5), (1, 1, 0, 1, 10),
        (0, 1, 0, 1, 4), (0, 0, 1, 1, 4), (1, 0, 1, 1, 4), (0, 0, 1, 1, 4),
        (0, 1, 0, 0, 9), (1, 1, 0, 0, 9), (0, 1, 0, 1, 6), (1, 1, 0, 1, 6),
        (1, 0, 0, 1, 6), (1, 1, 0, 1, 6),
    ]
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-async-reset-") as directory:
        work = Path(directory)
        # Settle combinational inputs before clock edges in both simulators.
        stimulus = []
        for clk, rstn, rst, enable, data in events:
            stimulus.append(f"""
    rstn <= '{rstn}'; rst <= '{rst}'; enable <= '{enable}';
    d <= std_ulogic_vector(to_unsigned({data}, 4)); wait for 1 ns;
    clk <= '{clk}'; wait for 1 ns;
    write(row, to_integer(unsigned(q))); write(row, string'(" "));
    write(row, to_integer(unsigned(p))); write(row, string'(" "));
    if held = '1' then write(row, 1); else write(row, 0); end if;
    writeline(trace, row);
""")
        work.joinpath("tb.vhd").write_text("""
library ieee; use ieee.std_logic_1164.all; use ieee.numeric_std.all;
use std.textio.all;
entity async_tb is end;
architecture tb of async_tb is
  signal clk, rst, enable : std_ulogic := '0';
  signal rstn : std_ulogic := '1';
  signal d, q : std_ulogic_vector(3 downto 0);
  signal p : std_ulogic_vector(0 to 3);
  signal held : std_ulogic;
begin
  dut: entity work.async_reset_test port map(clk, rstn, rst, enable, d, q, p, held);
  process
    file trace : text open write_mode is "trace.txt";
    variable row : line;
  begin
""" + "".join(stimulus) + "wait; end process; end;\n")
        run([args.nvc, "--std=2008", "-a", str(source), "tb.vhd"], work)
        run([args.nvc, "--std=2008", "-e", "async_tb"], work)
        run([args.nvc, "--std=2008", "-r", "async_tb", "--stop-time=30ns"], work)
        trace = work / "trace.txt"
        expected = trace.read_text().split()
        if len(expected) != 3 * len(events):
            raise RuntimeError("incomplete asynchronous-reset reference")
        run([adapter, "--gtest_filter=VHDLConstructorTest.AsynchronousResetPolarityValuesAndHoldCycles"],
            work, dict(os.environ, VHDL_ASYNC_RESET_REFERENCE=str(trace), VHDL_ASYNC_RESET_DUMP=str(work)))
        if args.iverilog and args.vvp:
            stimulus = []
            for clk, rstn, rst, enable, data in events:
                stimulus.append(f"""
    rstn = {rstn}; rst = {rst}; enable = {enable}; d = {data};
    #1; clk = {clk}; #1; $fdisplay(trace, "%0d %0d %0d", q, p, held);
""")
            work.joinpath("tb.v").write_text("""
module async_tb;
  reg clk = 0, rstn = 1, rst = 0, enable = 0;
  reg [3:0] d;
  wire [3:0] q;
  wire [0:3] p;
  wire held;
  integer trace;
  async_reset_test dut(.clk(clk), .rstn(rstn), .rst(rst), .enable(enable), .d(d), .q(q), .p(p), .held(held));
  initial begin
    trace = $fopen("dump_trace.txt", "w");
""" + "".join(stimulus) + "$fclose(trace); $finish; end\nendmodule\n")
            run([args.iverilog, "-g2012", "-s", "async_tb", "-o", "sim",
                 "async_reset_test.v", "naja_primitives.v", "tb.v"], work)
            run([args.vvp, "sim"], work)
            actual = work.joinpath("dump_trace.txt").read_text().split()
            if actual != expected:
                raise RuntimeError(f"exported asynchronous reset Verilog disagrees with NVC: {actual} != {expected}")
    print("Asynchronous reset, set, enable and omitted-bit hold behavior matches NVC.")


if __name__ == "__main__":
    main()
