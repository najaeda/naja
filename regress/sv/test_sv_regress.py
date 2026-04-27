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

    def test_materialize_fusesoc_vc_flist_filters_verilator_entries(self):
        case = {
            "name": "fake",
            "flist": "missing.vc",
            "flist_glob": "build/fusesoc/**/lint-verilator/*.vc",
            "flist_format": "fusesoc_vc",
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            case_dir = Path(tmpdir)
            repo_dir = case_dir / "repo"
            artifacts_dir = case_dir / "artifacts"
            vc_dir = repo_dir / "build" / "fusesoc" / "core" / "lint-verilator"
            (vc_dir / "src" / "rtl").mkdir(parents=True)
            artifacts_dir.mkdir()
            (vc_dir / "fake.vc").write_text(
                """
--Mdir .
+incdir+src/rtl
-CFLAGS -Isrc/rtl
src/lint/waiver.vlt
src/rtl/pkg.sv
src/rtl/top.sv
--top-module fake_top
-DSYNTHESIS=1
-DRVFI=1
""",
                encoding="utf-8",
            )

            generated = sv_regress.materialize_flist(case, repo_dir, case_dir, artifacts_dir)

            self.assertEqual(artifacts_dir / "fake.flist", generated)
            lines = generated.read_text(encoding="utf-8").splitlines()
            self.assertIn(f"+incdir+{vc_dir}/src/rtl", lines)
            self.assertIn(f"{vc_dir}/src/rtl/pkg.sv", lines)
            self.assertIn(f"{vc_dir}/src/rtl/top.sv", lines)
            self.assertIn("+define+SYNTHESIS=1", lines)
            self.assertIn("+define+RVFI=1", lines)
            self.assertFalse(any("waiver.vlt" in line for line in lines))
            self.assertFalse(any("--top-module" in line for line in lines))

    def test_run_setup_commands_formats_placeholders(self):
        case = {
            "name": "fake",
            "setup_timeout_seconds": 17,
            "setup_commands": [
                ["tool", "{repo}", "{case}", "{artifacts}", "{work}", "{naja}"],
            ],
            "env": {
                "FAKE_ROOT": "{repo}",
            },
        }
        commands = []

        def fake_run_command(command, **kwargs):
            commands.append((command, kwargs))

        original_run_command = sv_regress.run_command
        try:
            sv_regress.run_command = fake_run_command
            with tempfile.TemporaryDirectory() as tmpdir:
                case_dir = Path(tmpdir) / "case"
                repo_dir = case_dir / "repo"
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"

                sv_regress.run_setup_commands(
                    case,
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    log_dir=log_dir,
                )
        finally:
            sv_regress.run_command = original_run_command

        self.assertEqual(1, len(commands))
        command, kwargs = commands[0]
        self.assertEqual("tool", command[0])
        self.assertEqual(str(repo_dir), command[1])
        self.assertEqual(str(case_dir), command[2])
        self.assertEqual(str(artifacts_dir), command[3])
        self.assertEqual(str(case_dir.parent), command[4])
        self.assertEqual(str(sv_regress.REPO_ROOT), command[5])
        self.assertEqual(repo_dir, kwargs["cwd"])
        self.assertEqual(17, kwargs["timeout"])
        self.assertEqual(str(repo_dir), kwargs["env"]["FAKE_ROOT"])

    def test_ibex_setup_uses_generic_prim_mapping(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        ibex = sv_regress.select_cases(cases, "ibex")[0]
        setup_commands = ibex.get("setup_commands", [])
        self.assertEqual(1, len(setup_commands))
        self.assertIn("--mapping=lowrisc:prim_generic:all:0.1", setup_commands[0])


if __name__ == "__main__":
    unittest.main()
