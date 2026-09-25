# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare retained process-variable state with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-retained-") as work:
        run([args.nvc, "--std=2008", "-a",
             str(fixtures / "retained_variables.vhd"),
             str(fixtures / "retained_variables_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "retained_variables_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "retained_variables_tb",
                      "--stop-time=40ns"], work)
        if "RETAINED_VARIABLES_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if len(trace.read_text().split()) != 7:
            raise RuntimeError("incomplete reference trace")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.RetainedVariableConnectivityAndCycles"],
            work, dict(os.environ, VHDL_RETAINED_VARIABLE_REFERENCE=str(trace)))
    print("Retained process-variable state matches the NVC cycle trace.")


if __name__ == "__main__":
    main()
