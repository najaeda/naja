# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare typed scalar logical lowering with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-logical-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "logical.vhd"),
             str(fixtures / "logical_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "logical_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "logical_tb",
                      "--stop-time=20ns"], work)
        if "LOGICAL_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if len(trace.read_text().split()) != 8:
            raise RuntimeError("incomplete reference trace")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.NestedLogicalExpressionConnectivity"],
            work, dict(os.environ, VHDL_LOGICAL_REFERENCE=str(trace)))
    print("Typed scalar logical lowering matches the NVC truth table.")


if __name__ == "__main__":
    main()
