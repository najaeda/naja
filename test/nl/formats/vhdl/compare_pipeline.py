# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0
"""Compare the adapter's canonical DFF network with NVC for both write orders."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def run(command, cwd, env=None):
    result = subprocess.run(command, cwd=cwd, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=60)
    print(result.stdout, end="")
    result.check_returncode()
    return result.stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nvc", required=True)
    parser.add_argument("--adapter", required=True)
    args = parser.parse_args()
    fixtures = Path(__file__).resolve().parent
    source = (fixtures / "pipeline.vhd").read_text()
    run([args.nvc, "--version"], fixtures)
    for reverse in (False, True):
        with tempfile.TemporaryDirectory(prefix="naja-vhdl-pipeline-") as work:
            rtl = source.replace("stage <= d;\n      q <= stage;",
                                 "q <= stage;\n      stage <= d;") if reverse else source
            Path(work, "pipeline.vhd").write_text(rtl)
            run([args.nvc, "--std=2008", "-a", "pipeline.vhd",
                 str(fixtures / "pipeline_tb.vhd")], work)
            run([args.nvc, "--std=2008", "-e", "pipeline_tb"], work)
            output = run([args.nvc, "--std=2008", "-r", "pipeline_tb",
                          "--stop-time=40ns"], work)
            if "PIPELINE_DONE" not in output:
                raise RuntimeError("reference did not complete")
            trace = Path(work, "trace.txt")
            if len(trace.read_text().splitlines()) != 7:
                raise RuntimeError("incomplete reference trace")
            run([args.adapter,
                 "--gtest_filter=VHDLConstructorTest.PipelineConnectivityAndCycles"],
                work, dict(os.environ, VHDL_PIPELINE_REFERENCE=str(trace)))
    print("Both source orders match the NVC cycle trace.")


if __name__ == "__main__":
    main()
