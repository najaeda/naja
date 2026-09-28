# SPDX-License-Identifier: Apache-2.0
"""Protect the NVC reference runner's expected-result checks."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from run_nvc import run_probe


class NVCRunnerTest(unittest.TestCase):
    def check_result(self, outcomes, probe=None, overrides=None):
        probe = probe or {"id": "sample", "sources": ["unused.vhd"], "top": "probe"}
        with tempfile.TemporaryDirectory() as directory, patch(
                "run_nvc.invoke", side_effect=outcomes):
            return run_probe("nvc", probe, Path(directory) / "sample", 1,
                             overrides or {})["passed"]

    @staticmethod
    def outcome(code=0, output="", timeout=False):
        return {"returncode": code, "output": output, "timeout": timeout}

    def test_success_requires_probe_marker(self):
        self.assertFalse(self.check_result([
            self.outcome(), self.outcome(), self.outcome(output="stopped")]))
        self.assertTrue(self.check_result([
            self.outcome(), self.outcome(),
            self.outcome(output="PROBE_PASS:sample")]))

    def test_version_override_accepts_known_nvc_diagnostic(self):
        probe = {"id": "missing_declaration", "sources": ["unused.vhd"],
                 "top": "probe", "failure": {"stages": ["analyze"],
                 "diagnostic": "no declaration"}}
        overrides = {"missing_declaration": {
            "stages": ["analyze"],
            "diagnostic": r"no visible declaration for\s+missing"}}
        self.assertTrue(self.check_result([
            self.outcome(1, "no visible declaration for MISSING")],
            probe, overrides))

    def test_unexpected_stage_crash_and_timeout_do_not_pass(self):
        failure = {"stages": ["run"], "diagnostic": "range"}
        probe = {"id": "sample", "sources": ["unused.vhd"], "top": "probe",
                 "failure": failure}
        self.assertFalse(self.check_result([
            self.outcome(1, "range")], probe))
        self.assertFalse(self.check_result([
            self.outcome(None, "range", True)], probe))
        self.assertFalse(self.check_result([
            self.outcome(-11, "range")], probe))


if __name__ == "__main__":
    unittest.main()
