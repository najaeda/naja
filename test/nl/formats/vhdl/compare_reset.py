# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare active-high synchronous reset behavior with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-reset-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "reset.vhd"),
             str(fixtures / "reset_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "reset_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "reset_tb",
                      "--stop-time=12ns"], work)
        if "RESET_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if trace.read_text().split() != ["1", "1", "0", "1"]:
            raise RuntimeError("unexpected synchronous-reset behavior")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.SynchronousResetWiringAndCycles"],
            work, dict(os.environ, VHDL_RESET_REFERENCE=str(trace)))
    print("Active-high synchronous reset behavior matches NVC.")


if __name__ == "__main__":
    main()
