# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Exercise table ordering, writable DATA and invalid addresses in dumped Verilog."""
import argparse
import os
from pathlib import Path
import tempfile

from compare_pipeline import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--adapter", required=True)
    parser.add_argument("--iverilog", required=True)
    parser.add_argument("--vvp", required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-tables-") as directory:
        work = Path(directory)
        run([str(Path(args.adapter).resolve()),
             "--gtest_filter=VHDLConstructorTest.IndexedTablePrimitivesAndSharing:"
             "VHDLConstructorTest.ConstantFoldingAndCombinationalSharing"],
            work, dict(os.environ, VHDL_TABLE_DUMP=str(work)))
        work.joinpath("tb.v").write_text("""
module tb;
  reg clk = 0;
  reg [2:0] addr;
  reg [0:0] narrow;
  reg [7:0] d;
  wire [7:0] direct_q, signal_q, duplicate_q, late_q, writable_q;
  wire [7:0] offset_q, descending_q, narrow_q;
  reg [7:0] expected, written, offset_expected;
  integer pattern, a;
  indexed_tables dut(.*);
  reg fa, fb, fs;
  reg [3:0] fv;
  wire [11:0] folded_y;
  reg [11:0] folded_expected;
  folded folded_dut(.a(fa), .b(fb), .s(fs), .v(fv), .y(folded_y));
  function [7:0] rom(input integer index);
    case (index)
      0: rom = 8'h52;
      1: rom = 8'h09;
      2: rom = 8'hA6;
      default: rom = 8'hxx;
    endcase
  endfunction
  initial begin
    for (pattern = 0; pattern < 256; pattern = pattern + 1) begin
      d = pattern; clk = 0; #5; clk = 1; #5;
      for (a = 0; a < 8; a = a + 1) begin
        addr = a; narrow = a & 1; #1;
        expected = rom(a); offset_expected = rom(a - 3);
        case (a)
          0: written = d;
          1: written = ~d;
          2: written = d ^ 8'hA5;
          default: written = 8'hxx;
        endcase
        if (direct_q !== expected || signal_q !== expected ||
            duplicate_q !== expected || late_q !== expected ||
            writable_q !== written || offset_q !== offset_expected ||
            descending_q !== offset_expected || narrow_q !== rom(a & 1))
          $fatal(1, "table mismatch: pattern=%0d address=%0d", pattern, a);
      end
    end
    addr = 3'bxxx; #1;
    if (direct_q !== 8'hxx || writable_q !== 8'hxx || offset_q !== 8'hxx)
      $fatal(1, "unknown address did not produce X");
    addr = 3'bzzz; #1;
    if (direct_q !== 8'hxx || writable_q !== 8'hxx || descending_q !== 8'hxx)
      $fatal(1, "high-impedance address did not produce X");
    for (pattern = 0; pattern < 128; pattern = pattern + 1) begin
      fa = pattern & 1; fb = (pattern >> 1) & 1;
      fs = (pattern >> 2) & 1; fv = pattern >> 3; #1;
      folded_expected = {fv == 2, fv == 2, fa & fb, fa & fb,
                         fa, fa, fa, ~fa, 1'b1, fa, fa, 1'b0};
      if (folded_y !== folded_expected)
        $fatal(1, "folded output mismatch: pattern=%0d", pattern);
    end
    $display("PASS: 2048 table vectors plus X/Z addresses");
    $finish;
  end
endmodule
""")
        run([args.iverilog, "-g2012", "-s", "tb", "-o", "sim",
             "indexed_tables.v", "folded.v", "naja_primitives.v", "tb.v"], work)
        run([args.vvp, "sim"], work)
    print("Indexed table Verilog passed ordering, data and bounds checks.")


if __name__ == "__main__":
    main()
