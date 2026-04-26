# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import sys
import tempfile
import unittest

REGRESS_SV_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(REGRESS_SV_ROOT))

import sv_regress


class SVRegressRunnerTest(unittest.TestCase):
    def test_load_manifest_and_select_case(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            manifest = Path(tmpdir) / "cases.yml"
            manifest.write_text(
                """
cases:
  - name: fake
    repo: https://example.invalid/repo.git
    commit: 0123456789abcdef
    top: fake_top
    flist: rtl/files.f
    output: fake_naja.v
    verilator:
      image: verilator/verilator:v5.046
      suppressions:
        - PINMISSING
""",
                encoding="utf-8",
            )

            cases = sv_regress.load_manifest(manifest)

        self.assertEqual(["fake"], [case["name"] for case in cases])
        self.assertEqual(cases, sv_regress.select_cases(cases, "all"))
        self.assertEqual([cases[0]], sv_regress.select_cases(cases, "fake"))
        with self.assertRaises(sv_regress.RegressError):
            sv_regress.select_cases(cases, "missing")

    def test_run_verilator_uses_manifest_suppressions(self):
        case = {
            "name": "fake",
            "top": "fake_top",
            "verilator": {
                "image": "verilator/verilator:v5.046",
                "timeout_seconds": 12,
                "suppressions": ["PINMISSING", "MULTIDRIVEN"],
                "extra_args": ["--timing"],
            },
        }
        commands = []

        def fake_run_command(command, **kwargs):
            commands.append((command, kwargs))

        original_run_command = sv_regress.run_command
        try:
            sv_regress.run_command = fake_run_command
            with tempfile.TemporaryDirectory() as tmpdir:
                case_dir = Path(tmpdir)
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"
                artifacts_dir.mkdir(parents=True)
                generated = artifacts_dir / "fake_naja.v"
                generated.write_text("module fake_top; endmodule\n", encoding="utf-8")

                sv_regress.run_verilator(
                    case,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated,
                    log_dir=log_dir,
                )
        finally:
            sv_regress.run_command = original_run_command

        self.assertEqual(1, len(commands))
        command, kwargs = commands[0]
        self.assertIn("-Wno-PINMISSING", command)
        self.assertIn("-Wno-MULTIDRIVEN", command)
        self.assertIn("--timing", command)
        self.assertIn("/work/artifacts/fake_naja.v", command)
        self.assertIn("/work/artifacts/najaeda_primitives.v", command)
        self.assertEqual(12, kwargs["timeout"])

    def test_materialize_flist_appends_manifest_entries(self):
        case = {
            "name": "fake",
            "flist": "rtl/files.f",
            "flist_append": [
                "$BP_DIR/rtl/extra.sv",
                "{repo}/rtl/absolute_extra.sv",
            ],
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            case_dir = Path(tmpdir)
            repo_dir = case_dir / "repo"
            artifacts_dir = case_dir / "artifacts"
            (repo_dir / "rtl").mkdir(parents=True)
            artifacts_dir.mkdir()
            (repo_dir / "rtl" / "files.f").write_text("base.sv\n", encoding="utf-8")

            generated = sv_regress.materialize_flist(case, repo_dir, case_dir, artifacts_dir)

            self.assertEqual(artifacts_dir / "fake.flist", generated)
            text = generated.read_text(encoding="utf-8")
            self.assertIn("base.sv\n", text)
            self.assertIn("$BP_DIR/rtl/extra.sv\n", text)
            self.assertIn(f"{repo_dir}/rtl/absolute_extra.sv\n", text)


if __name__ == "__main__":
    unittest.main()
