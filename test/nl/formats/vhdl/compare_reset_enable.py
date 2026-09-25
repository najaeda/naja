# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare synchronous-reset plus clock-enable behavior with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-reset-enable-") as work:
        run([args.nvc, "--std=2008", "-a",
             str(fixtures / "reset_enable.vhd"),
             str(fixtures / "reset_enable_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "reset_enable_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "reset_enable_tb",
                      "--stop-time=16ns"], work)
        if "RESET_ENABLE_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if trace.read_text().split() != ["1", "1", "1", "0", "0", "1"]:
            raise RuntimeError("unexpected reset-enable behavior")
        run([args.adapter,
             "--gtest_filter="
             "VHDLConstructorTest.SynchronousResetWithEnableWiringAndCycles"],
            work, dict(os.environ, VHDL_RESET_ENABLE_REFERENCE=str(trace)))
    print("Synchronous-reset plus clock-enable behavior matches NVC.")


if __name__ == "__main__":
    main()
