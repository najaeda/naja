# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare active-high synchronous clock-enable behavior with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-enable-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "enabled.vhd"),
             str(fixtures / "enabled_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "enabled_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "enabled_tb",
                      "--stop-time=10ns"], work)
        if "ENABLE_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if trace.read_text().split() != ["1", "1", "0"]:
            raise RuntimeError("unexpected enabled-register behavior")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.ClockEnableWiringAndCycles"],
            work, dict(os.environ, VHDL_ENABLE_REFERENCE=str(trace)))
    print("Active-high clock-enable behavior matches NVC.")


if __name__ == "__main__":
    main()
