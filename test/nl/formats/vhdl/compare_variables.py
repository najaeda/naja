# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare immediate variable and scheduled signal values with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-variables-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "variables.vhd"),
             str(fixtures / "variables_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "variables_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "variables_tb",
                      "--stop-time=40ns"], work)
        if "VARIABLES_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if len(trace.read_text().split()) != 21:
            raise RuntimeError("incomplete reference trace")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.VariableSchedulingConnectivityAndCycles"],
            work, dict(os.environ, VHDL_VARIABLE_REFERENCE=str(trace)))
    print("Immediate variables and scheduled signals match the NVC cycle trace.")


if __name__ == "__main__":
    main()
