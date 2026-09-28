# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare ascending/descending vector position mapping with NVC."""
import argparse
import os
from pathlib import Path
import tempfile

from compare_pipeline import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nvc", required=True)
    parser.add_argument("--adapter", required=True)
    args = parser.parse_args()
    args.adapter = str(Path(args.adapter).resolve())
    fixtures = Path(__file__).resolve().parent
    run([args.nvc, "--version"], fixtures)
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-vectors-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "vectors.vhd"),
             str(fixtures / "vectors_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "vectors_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "vectors_tb",
                      "--stop-time=10ns"], work)
        if "VECTORS_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if trace.read_text().split() != ["0110", "1001", "0011"]:
            raise RuntimeError("unexpected vector position mapping")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.PreservesVectorRangesAndPositionalMuxMapping"],
            work, dict(os.environ, VHDL_VECTOR_REFERENCE=str(trace)))
    print("Ascending and descending vector mapping matches NVC.")


if __name__ == "__main__":
    main()
