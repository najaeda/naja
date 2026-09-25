# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare direct-entity hierarchy and positional vector mapping with NVC."""
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
    with tempfile.TemporaryDirectory(prefix="naja-vhdl-hierarchy-") as work:
        run([args.nvc, "--std=2008", "-a", str(fixtures / "hierarchy.vhd"),
             str(fixtures / "hierarchy_tb.vhd")], work)
        run([args.nvc, "--std=2008", "-e", "hierarchy_tb"], work)
        output = run([args.nvc, "--std=2008", "-r", "hierarchy_tb",
                      "--stop-time=10ns"], work)
        if "HIERARCHY_DONE" not in output:
            raise RuntimeError("reference did not complete")
        trace = Path(work, "trace.txt")
        if trace.read_text().split() != ["1001", "0110", "0011"]:
            raise RuntimeError("unexpected hierarchy position mapping")
        run([args.adapter,
             "--gtest_filter=VHDLConstructorTest.LowersOneLevelDirectEntityHierarchy"],
            work, dict(os.environ, VHDL_HIERARCHY_REFERENCE=str(trace)))
    print("Direct-entity hierarchy mapping matches NVC.")


if __name__ == "__main__":
    main()
