# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
# SPDX-License-Identifier: Apache-2.0

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import pulp_benchmarks as pulp


class PulpBenchmarksTest(unittest.TestCase):
    def test_unsupported_warnings_and_incomplete_diagnostics_fail(self):
        success = "summary.status=success\n"
        pulp.validate_diagnostics(success + 'diagnostic stage="naja_elaboration" severity="warning"')
        for report in ("status=in_progress", success + 'severity="error"',
                       success + 'severity="fatal"',
                       success + 'stage="naja_unsupported_warning" severity="warning"',
                       success + 'stage="naja_unsupported_error" severity="warning"'):
            with self.subTest(report=report), self.assertRaises(RuntimeError):
                pulp.validate_diagnostics(report)

    def test_archive_checksum_is_required_before_extraction(self):
        with tempfile.TemporaryDirectory() as tmp:
            archive = Path(tmp) / "wrong.tar.gz"
            archive.write_bytes(b"not the pinned release")
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                pulp.unpack(archive, Path(tmp) / "rtl")
            self.assertFalse((Path(tmp) / "rtl").exists())

    def test_target_inventory_rejects_duplicates_and_empty_input(self):
        with tempfile.TemporaryDirectory() as tmp:
            package = Path(tmp)
            manifest = package / "targets.mk"
            for content in ("", "x.y.TOP := top\nx.y.TOP := other\n"):
                manifest.write_text(content)
                with self.assertRaises(ValueError):
                    pulp.targets(package)
            manifest.write_text("x.y.TOP := top\nz.w.TOP := other\n")
            self.assertEqual({"x.y": "top", "z.w": "other"}, pulp.targets(package))

    def test_failed_and_timed_out_workers_cannot_reuse_stale_outputs(self):
        for outcome, expected in ((subprocess.CompletedProcess([], 1), "failed"),
                                  (subprocess.TimeoutExpired([], 1), "timeout"),
                                  (subprocess.CompletedProcess([], 0), "failed")):
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                source = root / "design/variants/default"
                source.mkdir(parents=True)
                (source / "filelist.f").write_text("input.sv\n")
                artifacts = root / "artifacts"
                artifacts.mkdir()
                (artifacts / "netlist.v").write_text("stale")
                (artifacts / "design-stats.json").write_text("{}")
                with patch.object(pulp.subprocess, "run") as run:
                    if isinstance(outcome, Exception):
                        run.side_effect = outcome
                    else:
                        run.return_value = outcome
                    result = pulp.run_target(root, "design.default", "top", artifacts, root, 1)
                self.assertEqual(expected, result["status"])
                self.assertFalse((artifacts / "netlist.v").exists())
                self.assertEqual(expected, json.loads((artifacts / "summary.json").read_text())["status"])
                self.assertEqual(1, run.call_args.kwargs["timeout"])
                self.assertEqual(source, run.call_args.kwargs["cwd"])

    def test_lint_failure_is_not_a_load_dump_pass(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "design/variants/default"
            source.mkdir(parents=True)
            (source / "filelist.f").write_text("input.sv\n")
            artifacts = root / "artifacts"

            def execute(command, **kwargs):
                if command[0] == "verilator":
                    raise FileNotFoundError("verilator is missing")
                (artifacts / "netlist.v").write_text("module top; endmodule\n")
                (artifacts / "design-stats.json").write_text('{"top": "top"}')
                return subprocess.CompletedProcess(command, 0)

            with patch.object(pulp.subprocess, "run", side_effect=execute):
                result = pulp.run_target(root, "design.default", "top", artifacts, root, 1, "local")
            self.assertEqual("lint_failed", result["status"])
            self.assertIn("verilator is missing", result["error"])


if __name__ == "__main__":
    unittest.main()
