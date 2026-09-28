# SPDX-License-Identifier: Apache-2.0
"""Protect the reference suite against false passes, including expected failures."""

import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from run import run_probe


class RunnerTest(unittest.TestCase):
    def check_result(self, outcomes, failure=None):
        probe = {"id": "sample", "sources": ["unused.vhd"], "top": "probe"}
        if failure:
            probe["failure"] = failure
        with tempfile.TemporaryDirectory() as directory, patch("run.invoke", side_effect=outcomes):
            return run_probe("ghdl", probe, Path(directory) / "sample", 1)["passed"]

    @staticmethod
    def outcome(code=0, output="", timeout=False):
        return {"returncode": code, "output": output, "timeout": timeout}

    def test_requires_completion_marker(self):
        self.assertFalse(self.check_result([self.outcome(), self.outcome(),
                                           self.outcome(output="stopped by --stop-time")]))
        self.assertTrue(self.check_result([self.outcome(), self.outcome(),
                                          self.outcome(output="PROBE_PASS:sample")]))

    def test_wrong_case_marker_does_not_pass(self):
        self.assertFalse(self.check_result([self.outcome(), self.outcome(),
                                           self.outcome(output="PROBE_PASS:another")]))

    def test_failure_needs_expected_stage_and_diagnostic(self):
        failure = {"stages": ["run"], "diagnostic": "bound check"}
        self.assertFalse(self.check_result([self.outcome(1, "bound check")], failure))
        self.assertFalse(self.check_result([self.outcome(), self.outcome(),
                                           self.outcome(1, "missing library")], failure))
        self.assertTrue(self.check_result([self.outcome(), self.outcome(),
                                          self.outcome(1, "bound check failure")], failure))

    def test_expected_rejection_must_not_accept_source(self):
        failure = {"stages": ["analyze"], "diagnostic": "type"}
        self.assertFalse(self.check_result([self.outcome(), self.outcome(),
                                           self.outcome(output="PROBE_PASS:sample")], failure))

    def test_crash_and_timeout_never_count_as_expected_rejection(self):
        failure = {"stages": ["analyze"], "diagnostic": "type"}
        for code in (-11, 2):
            with self.subTest(code=code):
                self.assertFalse(self.check_result([self.outcome(code, "type")], failure))
        self.assertFalse(self.check_result([self.outcome(None, "type", True)], failure))


if __name__ == "__main__":
    unittest.main()
