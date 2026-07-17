# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import os
import sys
import tempfile
import unittest
import yaml

REGRESS_SV_ROOT = Path(__file__).resolve().parent
HELLOWORLD_SIM_ROOT = REGRESS_SV_ROOT / "helloworld_sim"
sys.path.insert(0, str(REGRESS_SV_ROOT))
sys.path.insert(0, str(HELLOWORLD_SIM_ROOT))

import cv32e40p_example_tb
import cv32e40p_variant_netlist
import cva6_testharness
import ibex_secure_netlist
import ibex_simple_system
import logic_cone_signature
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
        - MULTIDRIVEN
""",
                encoding="utf-8",
            )

            cases = sv_regress.load_manifest(manifest)

        self.assertEqual(["fake"], [case["name"] for case in cases])
        self.assertEqual(cases, sv_regress.select_cases(cases, "all"))
        self.assertEqual([cases[0]], sv_regress.select_cases(cases, "fake"))
        self.assertEqual(cases, sv_regress.select_requested_cases(cases, None))
        self.assertEqual([cases[0]], sv_regress.select_requested_cases(cases, ["fake"]))
        self.assertEqual(cases, sv_regress.select_requested_cases(cases, ["all"]))
        with self.assertRaises(sv_regress.RegressError):
            sv_regress.select_cases(cases, "missing")

    def test_parser_accumulates_repeated_case_options(self):
        parser = sv_regress.build_parser()
        args = parser.parse_args(["run", "--case", "ibex", "--case", "cv32e40p"])

        self.assertEqual(["ibex", "cv32e40p"], args.case)

    def test_stage_selection_defaults_and_deduplicates(self):
        self.assertEqual(["lint", "github_sim"], sv_regress.select_stages(None))
        self.assertEqual(
            [
                "load_dump",
                "lint",
                "logic_cones",
                "cva6_extended_sim",
                "cva6_full_sim",
                "cva6_local_verif_sim",
                "cv32e40p_hwlp_sim",
                "helloworld_sim",
                "ibex_dit_sim",
                "ibex_dummy_instr_sim",
                "ibex_pmp_sim",
                "ibex_secure_dit_sim",
                "ibex_secure_dummy_instr_sim",
                "interrupt_sim",
            ],
            sv_regress.select_stages([
                "load_dump",
                "lint",
                "logic_cones",
                "cva6_extended_sim",
                "cva6_full_sim",
                "cva6_local_verif_sim",
                "cv32e40p_hwlp_sim",
                "helloworld_sim",
                "ibex_dit_sim",
                "ibex_dummy_instr_sim",
                "ibex_pmp_sim",
                "ibex_secure_dit_sim",
                "ibex_secure_dummy_instr_sim",
                "interrupt_sim",
                "lint",
            ]),
        )
        with self.assertRaises(sv_regress.RegressError):
            sv_regress.select_stages(["missing"])

    def test_parser_accumulates_repeated_stage_options(self):
        parser = sv_regress.build_parser()
        args = parser.parse_args(["run", "--stage", "lint", "--stage", "helloworld_sim"])

        self.assertEqual(["lint", "helloworld_sim"], args.stage)

    def test_primitives_file_contains_table_select_shim(self):
        primitives = sv_regress.PRIMITIVES_PATH.read_text(encoding="utf-8")

        self.assertIn("module naja_table_select #(", primitives)
        self.assertIn("function [WIDTH-1:0] select_data", primitives)
        self.assertIn("if (addr == i[ABITS-1:0])", primitives)
        self.assertIn("assign Y = select_data(DATA, ADDR)", primitives)

    def test_memory_primitive_uses_bounded_disabled_init_default(self):
        primitives = sv_regress.PRIMITIVES_PATH.read_text(encoding="utf-8")

        self.assertIn("parameter INIT = 1'b0", primitives)
        self.assertIn("parameter RESET_VALUE = INIT", primitives)
        self.assertIn(
            "mem[reset_idx] = RESET_VALUE[reset_idx*WIDTH +: WIDTH]",
            primitives,
        )
        self.assertEqual(3, primitives.count("load_reset();"))
        self.assertNotIn("parameter [WIDTH*DEPTH-1:0] INIT", primitives)

    def test_sequential_primitives_accept_init_parameter(self):
        primitives = sv_regress.PRIMITIVES_PATH.read_text(encoding="utf-8")

        self.assertIn("module naja_dff #(\n  parameter WIDTH = 1,\n  parameter INIT = {WIDTH{1'bx}}", primitives)
        self.assertIn("module naja_dffn #(\n  parameter WIDTH = 1,\n  parameter INIT = {WIDTH{1'bx}}", primitives)
        self.assertIn("module naja_dffre #(\n  parameter WIDTH = 1,\n  parameter INIT = {WIDTH{1'bx}}", primitives)
        self.assertIn("initial Q = INIT;", primitives)

    def test_parser_accepts_local_lint_runner(self):
        parser = sv_regress.build_parser()
        args = parser.parse_args(["run", "--lint-runner", "local"])

        self.assertEqual("local", args.lint_runner)

    def test_parser_accepts_firmware_tool_requirement_aliases(self):
        parser = sv_regress.build_parser()

        args = parser.parse_args(["run", "--require-firmware-sim-tools"])
        legacy_args = parser.parse_args(["run", "--require-helloworld-sim-tools"])
        expected_failure_args = parser.parse_args(["run", "--allow-expected-failures"])

        self.assertTrue(args.require_firmware_sim_tools)
        self.assertTrue(legacy_args.require_firmware_sim_tools)
        self.assertTrue(expected_failure_args.allow_expected_failures)

    def test_run_load_dump_records_generated_verilog(self):
        case = {
            "name": "fake",
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)
            generated = tmpdir_path / "artifacts" / "fake_naja.v"
            log_dir = tmpdir_path / "artifacts" / "logs"
            generated.parent.mkdir(parents=True)
            log_dir.mkdir()
            generated.write_text("module fake; endmodule\n", encoding="utf-8")

            result = sv_regress.run_load_dump(
                case,
                generated_path=generated,
                log_dir=log_dir,
            )

        self.assertEqual("passed", result["status"])
        self.assertEqual(str(generated), result["generated_verilog"])
        self.assertEqual(str(log_dir / "load-dump.log"), result["log"])

    def test_large_cases_use_load_dump_stage_in_ci(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        for case_name in ("black_parrot", "cva6"):
            case = sv_regress.select_cases(cases, case_name)[0]
            self.assertIn("output", case)
            self.assertIn("dump", case)

        workflow = (
            sv_regress.REPO_ROOT / ".github" / "workflows" / "sv-regress.yml"
        ).read_text(encoding="utf-8")
        self.assertIn("--case black_parrot --stage load_dump", workflow)
        self.assertIn(
            "--case cva6 --stage load_dump --stage logic_cones", workflow
        )

    def test_zcore_runs_full_dump_verification_flow_in_ci(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        zcore = sv_regress.select_cases(cases, "zcore")[0]

        self.assertEqual("z_core_top", zcore["top"])
        self.assertEqual("rtl/flist.vc", zcore["flist"])
        self.assertNotIn("verification_top", zcore)
        self.assertEqual("tb_zcore_top_smoke", zcore["github_sim"]["top_module"])
        self.assertEqual("ZCORE_TOP_SMOKE_PASS", zcore["github_sim"]["pass_regex"])

        workflow = (
            sv_regress.REPO_ROOT / ".github" / "workflows" / "sv-regress.yml"
        ).read_text(encoding="utf-8")
        self.assertIn("--case zcore", workflow)
        for stage in ("load_dump", "lint", "github_sim"):
            self.assertIn(f"--stage {stage}", workflow)

    def test_external_sim_ci_runs_cv32e40p_firmware_sims(self):
        workflow = (
            sv_regress.REPO_ROOT / ".github" / "workflows" / "sv-external-sim.yml"
        ).read_text(encoding="utf-8")
        workflow_config = yaml.safe_load(workflow)

        self.assertNotIn("schedule:", workflow)
        self.assertNotIn("cron:", workflow)
        self.assertIn("packages: read", workflow)
        self.assertIn("Run Ibex helloworld simulation", workflow)
        self.assertIn("Run Ibex extended simple-system diagnostics", workflow)
        self.assertIn("Run CV32E40P helloworld simulation", workflow)
        self.assertIn("Run CV32E40P extended example TB simulations", workflow)
        self.assertIn("cva6-pr-sim:", workflow)
        self.assertNotIn("cva6-full-sim:", workflow)
        self.assertIn("ghcr.io/najaeda/naja/sv-sim:latest", workflow)
        job = workflow_config["jobs"]["cva6-pr-sim"]
        self.assertIn("pull_request", job["if"])
        self.assertIn("push", job["if"])
        self.assertIn("workflow_dispatch", job["if"])
        self.assertNotIn("container", job)
        self.assertEqual(
            "ghcr.io/najaeda/naja/sv-sim:latest",
            job["env"]["SV_SIM_IMAGE"],
        )
        self.assertIn("Pull or build SV simulation image", workflow)
        self.assertIn("Log in to GHCR", workflow)
        self.assertIn("continue-on-error: true", workflow)
        self.assertIn("Set up Docker Buildx", workflow)
        self.assertIn('docker pull "${SV_SIM_IMAGE}"', workflow)
        self.assertIn("docker buildx build", workflow)
        self.assertIn("--load", workflow)
        self.assertIn("--cache-from type=gha,scope=sv-sim", workflow)
        self.assertIn("--cache-to type=gha,mode=max,scope=sv-sim", workflow)
        self.assertIn("-f docker/Dockerfile.sv-sim", workflow)
        self.assertIn('-t "${SV_SIM_IMAGE}"', workflow)
        self.assertIn("docker run --rm", workflow)
        self.assertIn("Run CVA6 full testharness simulation", workflow)
        self.assertNotIn("Run CVA6 extended testharness simulation", workflow)
        self.assertIn("xpack-riscv-none-elf-gcc", workflow)
        self.assertNotIn("riscv-isa-sim", workflow)
        self.assertIn("ccache", workflow)
        self.assertIn('--user "$(id -u):$(id -g)"', workflow)
        self.assertIn("CCACHE_DIR", workflow)
        self.assertIn(".docker-home", workflow)
        self.assertIn("libboost-dev", workflow)
        self.assertNotIn("libboost-system-dev", workflow)
        self.assertNotIn("VERILATOR_INSTALL_DIR", workflow)
        self.assertNotIn("SPIKE_INSTALL_DIR", workflow)
        self.assertIn("SV_REGRESS_SEED_ROOT", workflow)
        self.assertIn("NUM_JOBS: \"2\"", workflow)
        self.assertNotIn("picolibc-riscv64-unknown-elf", workflow)
        self.assertIn("--case ibex", workflow)
        self.assertIn("--case cv32e40p", workflow)
        self.assertIn("--case cva6_testharness", workflow)
        self.assertIn("--stage helloworld_sim", workflow)
        self.assertIn("--stage cva6_full_sim", workflow)
        self.assertNotIn("--stage cva6_extended_sim", workflow)
        self.assertNotIn("--stage cva6_local_verif_sim", workflow)
        self.assertIn("--stage ibex_pmp_sim", workflow)
        self.assertIn("--stage ibex_dit_sim", workflow)
        self.assertIn("--stage ibex_dummy_instr_sim", workflow)
        self.assertNotIn("--stage ibex_secure_dit_sim", workflow)
        self.assertNotIn("--stage ibex_secure_dummy_instr_sim", workflow)
        self.assertIn("--stage interrupt_sim", workflow)
        self.assertIn("--stage cv32e40p_hwlp_sim", workflow)
        self.assertIn("--require-firmware-sim-tools", workflow)
        self.assertNotIn("--allow-expected-failures", workflow)

    def test_sv_sim_image_installs_spike_boost_asio_dependencies(self):
        dockerfile = (
            sv_regress.REPO_ROOT / "docker" / "Dockerfile.sv-sim"
        ).read_text(encoding="utf-8")

        self.assertIn("libboost-dev", dockerfile)
        self.assertIn("libboost-system-dev", dockerfile)

    def test_cva6_testharness_has_helloworld_sim_stage(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        cva6 = sv_regress.select_cases(cases, "cva6_testharness")[0]

        self.assertIn("helloworld_sim", cva6)
        self.assertIn("cva6_extended_sim", cva6)
        self.assertIn("cva6_local_verif_sim", cva6)
        self.assertIn("cva6_full_sim", cva6)
        self.assertIn("cva6_full_sim", sv_regress.VALID_STAGES)
        self.assertIn("CVA6_HELLOWORLD_SIM_PASS", cva6["helloworld_sim"]["pass_regex"])
        self.assertNotIn("expected_failure", cva6["helloworld_sim"])
        self.assertNotIn("expected_failure_reason", cva6["helloworld_sim"])
        command = cva6["helloworld_sim"]["commands"][0]
        self.assertIn("cva6_testharness.py", command[1])
        self.assertIn("{artifacts}/cva6_testharness_naja.v", command)
        extended_command = cva6["cva6_extended_sim"]["commands"][0]
        self.assertIn("cva6_testharness.py", extended_command[1])
        self.assertIn("--program", extended_command)
        self.assertIn("hello_world", extended_command)
        self.assertIn("corev_dhrystone", extended_command)
        self.assertIn("--max-cycles", extended_command)
        self.assertIn("10000000", extended_command)
        local_command = cva6["cva6_local_verif_sim"]["commands"][0]
        self.assertIn("corev_return0", local_command)
        self.assertIn("corev_custom_template", local_command)
        self.assertIn("corev_isacov_branch_to_zero", local_command)
        self.assertIn("corev_isacov_jump", local_command)
        self.assertIn("corev_isacov_isa", local_command)
        self.assertIn("corev_isacov_seq_hazard", local_command)
        self.assertIn("corev_pmp_exact_csrr", local_command)
        self.assertIn("corev_pmp_granularity", local_command)
        self.assertIn("corev_pmp_lsu_tor", local_command)
        full_command = cva6["cva6_full_sim"]["commands"][0]
        for program in (
            "hello_world",
            "corev_dhrystone",
            "corev_return0",
            "corev_custom_template",
            "corev_isacov_branch_to_zero",
            "corev_isacov_jump",
            "corev_isacov_isa",
            "corev_isacov_seq_hazard",
            "corev_pmp_exact_csrr",
            "corev_pmp_granularity",
            "corev_pmp_lsu_tor",
        ):
            self.assertIn(program, full_command)
        self.assertEqual(
            cva6["helloworld_sim"]["pass_regex"],
            cva6["cva6_extended_sim"]["pass_regex"],
        )
        self.assertEqual(
            cva6["helloworld_sim"]["pass_regex"],
            cva6["cva6_full_sim"]["pass_regex"],
        )

    def test_cva6_has_pinned_logic_cone_signatures(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        cva6 = sv_regress.select_cases(cases, "cva6")[0]

        self.assertIn("logic_cones", cva6)
        self.assertEqual(
            [None, 468, 469],
            [probe.get("bit") for probe in cva6["logic_cones"]],
        )
        self.assertEqual(
            {
                "nodes": 18377,
                "edges": 34009,
                "leaves": 93,
                "roots": 471,
                "registers": 86,
                "ports": 3,
                "blackboxes": 4,
                "internal": 17813,
            },
            cva6["logic_cones"][0]["expected"],
        )
        for probe in cva6["logic_cones"][1:]:
            self.assertEqual(
                {
                    "nodes": 3,
                    "edges": 2,
                    "leaves": 0,
                    "roots": 1,
                    "registers": 0,
                    "ports": 0,
                    "blackboxes": 0,
                    "internal": 2,
                },
                probe["expected"],
            )

    def test_logic_cone_signature_counts_and_validation(self):
        class FakeCone:
            def get_nodes(self):
                return (
                    (0, None, "root", (1,), ()),
                    (1, None, "internal", (2, 3), (0,)),
                    (2, None, "flop", (), (1,)),
                    (3, None, "ports", (), (1,)),
                )

            def get_leaves(self):
                return self.get_nodes()[2:]

        signature = logic_cone_signature.summarize_cone(FakeCone())
        self.assertEqual(
            {
                "nodes": 4,
                "edges": 3,
                "leaves": 2,
                "roots": 1,
                "registers": 1,
                "ports": 1,
                "blackboxes": 0,
                "internal": 1,
            },
            signature,
        )
        probe = {"term": "out", "expected": signature}
        actual = {"root": "out", "direction": "fanin", **signature}
        logic_cone_signature.validate_signatures([probe], [actual])
        probe["expected"]["nodes"] = 5
        with self.assertRaisesRegex(RuntimeError, "out nodes: expected 5, got 4"):
            logic_cone_signature.validate_signatures([probe], [actual])

    def test_logic_cone_signature_accepts_whole_bus_root(self):
        class FakeBus:
            def getBusTermBit(self, _bit):
                raise AssertionError("whole-bus probe must not select a bit")

        class FakeTerm:
            def __init__(self):
                self.bus = FakeBus()

            def get_snl_term(self):
                return self.bus

        class FakeTop:
            def __init__(self):
                self.term = FakeTerm()

            def get_term(self, name):
                return self.term if name == "BUS" else None

        class FakeCone:
            def get_nodes(self):
                return ((0, None, "root", (), ()),)

            def get_leaves(self):
                return ()

        class FakeNaja:
            @staticmethod
            def SNLOccurrence(term):
                return term

            @staticmethod
            def LogicCone(_start, _direction):
                return FakeCone()

        signatures = logic_cone_signature.build_signatures(
            FakeTop(), [{"term": "BUS", "direction": "fanin"}], FakeNaja
        )

        self.assertEqual("BUS", signatures[0]["root"])
        self.assertEqual(1, signatures[0]["nodes"])

    def test_run_logic_cones_records_signature(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            artifacts = Path(tmpdir)
            signature_path = artifacts / "logic-cones.json"
            signature_path.write_text(
                '{"top":"fake","cones":[{"root":"out","nodes":4}]}\n',
                encoding="utf-8",
            )
            result = sv_regress.run_logic_cones(
                {
                    "name": "fake",
                    "logic_cones": [{"term": "out", "expected": {"nodes": 4}}],
                },
                artifacts_dir=artifacts,
            )

        self.assertEqual("passed", result["status"])
        self.assertEqual([{"root": "out", "nodes": 4}], result["cones"])

    def test_run_verilator_uses_manifest_suppressions(self):
        case = {
            "name": "fake",
            "top": "fake_top",
            "verilator": {
                "image": "verilator/verilator:v5.046",
                "timeout_seconds": 12,
                "suppressions": ["MULTIDRIVEN", "WIDTH"],
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
        self.assertIn("-Wno-MULTIDRIVEN", command)
        self.assertIn("-Wno-WIDTH", command)
        self.assertIn("--timing", command)
        self.assertIn("/work/artifacts/fake_naja.v", command)
        self.assertIn("/work/artifacts/najaeda_primitives.v", command)
        self.assertEqual(12, kwargs["timeout"])
        self.assertEqual(log_dir / "verilator-lint.log", kwargs["log_path"])

    def test_run_lint_can_use_local_verilator(self):
        case = {
            "name": "fake",
            "top": "fake_top",
            "verilator": {
                "image": "verilator/verilator:v5.046",
                "timeout_seconds": 12,
                "suppressions": ["MULTIDRIVEN"],
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

                sv_regress.run_lint(
                    case,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated,
                    log_dir=log_dir,
                    lint_runner="local",
                )
        finally:
            sv_regress.run_command = original_run_command

        self.assertEqual(1, len(commands))
        command, kwargs = commands[0]
        self.assertEqual("verilator", command[0])
        self.assertNotIn("docker", command)
        self.assertIn("--lint-only", command)
        self.assertIn("-Wno-MULTIDRIVEN", command)
        self.assertIn(str(generated), command)
        self.assertIn(str(artifacts_dir / "najaeda_primitives.v"), command)
        self.assertEqual(12, kwargs["timeout"])

    def test_run_github_sim_builds_and_runs_binary(self):
        case = {
            "name": "fake",
            "top": "fake_top",
            "verilator": {
                "image": "verilator/verilator:v5.046",
                "timeout_seconds": 12,
                "suppressions": ["MULTIDRIVEN"],
            },
            "github_sim": {
                "image": "verilator/verilator:v5.046",
                "timeout_seconds": 34,
                "top_module": "tb_fake",
                "sources": ["{case}/tb_fake.sv"],
                "suppressions": ["PINCONNECTEMPTY"],
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
                repo_dir = case_dir / "repo"
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"
                artifacts_dir.mkdir(parents=True)
                repo_dir.mkdir()
                generated = artifacts_dir / "fake_naja.v"
                generated.write_text("module fake_top; endmodule\n", encoding="utf-8")
                (case_dir / "tb_fake.sv").write_text("module tb_fake; endmodule\n", encoding="utf-8")

                sv_regress.run_verilator_binary_stage(
                    case,
                    stage="github_sim",
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated,
                    log_dir=log_dir,
                )
        finally:
            sv_regress.run_command = original_run_command

        self.assertEqual(2, len(commands))
        build_command, build_kwargs = commands[0]
        run_command, run_kwargs = commands[1]
        self.assertIn("--binary", build_command)
        self.assertIn("--top-module", build_command)
        self.assertIn("tb_fake", build_command)
        self.assertIn("-Wno-MULTIDRIVEN", build_command)
        self.assertIn("-Wno-PINCONNECTEMPTY", build_command)
        self.assertIn("/work/artifacts/fake_naja.v", build_command)
        self.assertTrue(any(arg.endswith("/tb_fake.sv") for arg in build_command))
        self.assertEqual(34, build_kwargs["timeout"])
        self.assertEqual(log_dir / "github-sim.log", build_kwargs["log_path"])
        self.assertIn("/work/artifacts/github_sim-obj/Vtb_fake", run_command)
        self.assertTrue(run_kwargs["append_log"])

    def test_run_helloworld_sim_can_use_local_verilator_smoke_stage(self):
        case = {
            "name": "fake",
            "top": "fake_top",
            "output": "fake_naja.v",
            "verilator": {
                "suppressions": ["MULTIDRIVEN"],
            },
            "helloworld_sim": {
                "runner": "local",
                "timeout_seconds": 34,
                "top_module": "tb_fake",
                "sources": ["{case}/tb_fake.sv"],
                "suppressions": ["PINCONNECTEMPTY"],
            },
        }
        commands = []

        def fake_run_command(command, **kwargs):
            commands.append((command, kwargs))

        original_run_command = sv_regress.run_command
        original_which = sv_regress.shutil.which
        try:
            sv_regress.run_command = fake_run_command
            sv_regress.shutil.which = lambda tool: f"/usr/bin/{tool}"
            with tempfile.TemporaryDirectory() as tmpdir:
                case_dir = Path(tmpdir)
                repo_dir = case_dir / "repo"
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"
                artifacts_dir.mkdir(parents=True)
                repo_dir.mkdir()
                generated = artifacts_dir / "fake_naja.v"
                generated.write_text("module fake_top; endmodule\n", encoding="utf-8")
                (case_dir / "tb_fake.sv").write_text("module tb_fake; endmodule\n", encoding="utf-8")

                result = sv_regress.run_helloworld_sim(
                    case,
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=generated,
                    log_dir=log_dir,
                    require_tools=False,
                )
        finally:
            sv_regress.run_command = original_run_command
            sv_regress.shutil.which = original_which

        self.assertEqual("passed", result["status"])
        self.assertEqual(2, len(commands))
        build_command, build_kwargs = commands[0]
        run_command, run_kwargs = commands[1]
        self.assertEqual("verilator", build_command[0])
        self.assertIn("--binary", build_command)
        self.assertIn("-Wno-MULTIDRIVEN", build_command)
        self.assertIn("-Wno-PINCONNECTEMPTY", build_command)
        self.assertIn(str(generated), build_command)
        self.assertIn(str(artifacts_dir / "helloworld_sim-obj" / "Vtb_fake"), run_command)
        self.assertEqual(log_dir / "helloworld-sim.log", build_kwargs["log_path"])
        self.assertTrue(run_kwargs["append_log"])

    def test_helloworld_sim_skips_when_optional_tool_is_missing(self):
        case = {
            "name": "fake",
            "helloworld_sim": {
                "tool_checks": ["definitely-missing-riscv-tool"],
            },
        }
        with tempfile.TemporaryDirectory() as tmpdir:
            case_dir = Path(tmpdir) / "case"
            repo_dir = case_dir / "repo"
            artifacts_dir = case_dir / "artifacts"
            log_dir = artifacts_dir / "logs"
            log_dir.mkdir(parents=True)

            result = sv_regress.run_helloworld_sim(
                case,
                repo_dir=repo_dir,
                case_dir=case_dir,
                artifacts_dir=artifacts_dir,
                generated_path=artifacts_dir / "fake_naja.v",
                log_dir=log_dir,
                require_tools=False,
            )

        self.assertEqual("skipped", result["status"])
        self.assertIn("definitely-missing-riscv-tool", result["reason"])

    def test_expected_failure_configured_sim_can_be_allowed(self):
        case = {
            "name": "fake",
            "interrupt_sim": {
                "expected_failure": True,
                "expected_failure_reason": "known interrupt gap",
                "commands": [["run-interrupt"]],
            },
        }

        def fake_run_command(command, **kwargs):
            raise sv_regress.RegressError("interrupt timed out")

        original_run_command = sv_regress.run_command
        try:
            sv_regress.run_command = fake_run_command
            with tempfile.TemporaryDirectory() as tmpdir:
                case_dir = Path(tmpdir) / "case"
                repo_dir = case_dir / "repo"
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"
                repo_dir.mkdir(parents=True)
                log_dir.mkdir(parents=True)

                result = sv_regress.run_configured_command_sim(
                    case,
                    stage="interrupt_sim",
                    repo_dir=repo_dir,
                    case_dir=case_dir,
                    artifacts_dir=artifacts_dir,
                    generated_path=artifacts_dir / "fake_naja.v",
                    log_dir=log_dir,
                    require_tools=False,
                    allow_expected_failures=True,
                )
        finally:
            sv_regress.run_command = original_run_command

        self.assertEqual("expected_failure", result["status"])
        self.assertEqual("known interrupt gap", result["reason"])

    def test_expected_failure_configured_sim_still_fails_by_default(self):
        case = {
            "name": "fake",
            "interrupt_sim": {
                "expected_failure": True,
                "expected_failure_reason": "known interrupt gap",
                "commands": [["run-interrupt"]],
            },
        }

        def fake_run_command(command, **kwargs):
            raise sv_regress.RegressError("interrupt timed out")

        original_run_command = sv_regress.run_command
        try:
            sv_regress.run_command = fake_run_command
            with tempfile.TemporaryDirectory() as tmpdir:
                case_dir = Path(tmpdir) / "case"
                repo_dir = case_dir / "repo"
                artifacts_dir = case_dir / "artifacts"
                log_dir = artifacts_dir / "logs"
                repo_dir.mkdir(parents=True)
                log_dir.mkdir(parents=True)

                with self.assertRaisesRegex(
                    sv_regress.RegressError,
                    "--allow-expected-failures",
                ):
                    sv_regress.run_configured_command_sim(
                        case,
                        stage="interrupt_sim",
                        repo_dir=repo_dir,
                        case_dir=case_dir,
                        artifacts_dir=artifacts_dir,
                        generated_path=artifacts_dir / "fake_naja.v",
                        log_dir=log_dir,
                        require_tools=False,
                    )
        finally:
            sv_regress.run_command = original_run_command

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

    def test_seed_checkout_uses_sv_regress_seed_root_case_directory(self):
        original_seed_root = os.environ.get("SV_REGRESS_SEED_ROOT")
        try:
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                seed_root = tmpdir_path / "seeds"
                seed_dir = seed_root / "fake"
                repo_dir = tmpdir_path / "work" / "fake" / "repo"
                log_dir = tmpdir_path / "work" / "fake" / "artifacts" / "logs"
                (seed_dir / ".git").mkdir(parents=True)
                (seed_dir / "rtl.sv").write_text("module fake; endmodule\n", encoding="utf-8")
                os.environ["SV_REGRESS_SEED_ROOT"] = str(seed_root)

                seeded = sv_regress.seed_checkout({"name": "fake"}, repo_dir, log_dir)

                self.assertTrue(seeded)
                self.assertTrue((repo_dir / ".git").exists())
                self.assertEqual(
                    "module fake; endmodule\n",
                    (repo_dir / "rtl.sv").read_text(encoding="utf-8"),
                )
                self.assertIn(
                    f"Seeded checkout for fake from {seed_dir}",
                    (log_dir / "seed-checkout.log").read_text(encoding="utf-8"),
                )
        finally:
            if original_seed_root is None:
                os.environ.pop("SV_REGRESS_SEED_ROOT", None)
            else:
                os.environ["SV_REGRESS_SEED_ROOT"] = original_seed_root

    def test_ibex_setup_uses_generic_prim_mapping(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        ibex = sv_regress.select_cases(cases, "ibex")[0]
        setup_commands = ibex.get("setup_commands", [])
        self.assertEqual(1, len(setup_commands))
        self.assertIn("--mapping=lowrisc:prim_generic:all:0.1", setup_commands[0])

    def test_cv32e40p_case_uses_rtl_manifest_and_design_dir(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        cv32e40p = sv_regress.select_cases(cases, "cv32e40p")[0]

        self.assertEqual("https://github.com/openhwgroup/cv32e40p.git", cv32e40p["repo"])
        self.assertEqual("cv32e40p_top", cv32e40p["top"])
        self.assertEqual("cv32e40p_manifest.flist", cv32e40p["flist"])
        self.assertEqual("{repo}/rtl", cv32e40p["env"]["DESIGN_RTL_DIR"])

    def test_smoke_cases_have_github_and_helloworld_sim_stages(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        for case_name, smoke_pass_regex, helloworld_pass_regex, helper in [
            (
                "ibex",
                "IBEX_SMOKE_PASS",
                "IBEX_HELLOWORLD_SIM_PASS",
                "regress/sv/helloworld_sim/ibex_simple_system.py",
            ),
            (
                "cv32e40p",
                "CV32E40P_SMOKE_PASS",
                "CV32E40P_HELLOWORLD_SIM_PASS",
                "regress/sv/helloworld_sim/cv32e40p_example_tb.py",
            ),
        ]:
            case = sv_regress.select_cases(cases, case_name)[0]
            self.assertEqual(smoke_pass_regex, case["github_sim"]["pass_regex"])
            self.assertEqual(helloworld_pass_regex, case["helloworld_sim"]["pass_regex"])
            self.assertIn("top_module", case["github_sim"])
            self.assertIn("commands", case["helloworld_sim"])
            self.assertIn(helper, case["helloworld_sim"]["commands"][0][1])
            self.assertIn(
                [
                    "riscv32-unknown-elf-gcc",
                    "riscv-none-elf-gcc",
                    "riscv64-unknown-elf-gcc",
                ],
                case["helloworld_sim"]["tool_checks"],
            )
            self.assertIn(
                [
                    "riscv32-unknown-elf-objcopy",
                    "riscv-none-elf-objcopy",
                    "riscv64-unknown-elf-objcopy",
                ],
                case["helloworld_sim"]["tool_checks"],
            )

    def test_cv32e40p_interrupt_sim_stage_uses_upstream_example_tb(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        cv32e40p = sv_regress.select_cases(cases, "cv32e40p")[0]
        interrupt_sim = cv32e40p["interrupt_sim"]

        self.assertIn("interrupt_sim", sv_regress.VALID_STAGES)
        self.assertEqual("CV32E40P_INTERRUPT_SIM_PASS", interrupt_sim["pass_regex"])
        self.assertIn("regress/sv/helloworld_sim/cv32e40p_example_tb.py",
                      interrupt_sim["commands"][0][1])
        self.assertIn("--program", interrupt_sim["commands"][0])
        self.assertIn("interrupt", interrupt_sim["commands"][0])
        self.assertIn("--max-cycles", interrupt_sim["commands"][0])
        self.assertIn("20000000", interrupt_sim["commands"][0])
        self.assertNotIn("expected_failure", interrupt_sim)
        self.assertNotIn("expected_failure_reason", interrupt_sim)
        self.assertIn(
            [
                "riscv32-unknown-elf-gcc",
                "riscv-none-elf-gcc",
                "riscv64-unknown-elf-gcc",
            ],
            interrupt_sim["tool_checks"],
        )

    def test_cv32e40p_hwlp_sim_stage_uses_pulp_netlist_variant(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        cv32e40p = sv_regress.select_cases(cases, "cv32e40p")[0]
        hwlp_sim = cv32e40p["cv32e40p_hwlp_sim"]

        self.assertIn("cv32e40p_hwlp_sim", sv_regress.VALID_STAGES)
        self.assertEqual("CV32E40P_HWLP_SIM_PASS", hwlp_sim["pass_regex"])
        self.assertNotIn("expected_failure", hwlp_sim)
        self.assertEqual(2, len(hwlp_sim["commands"]))
        self.assertIn("hwlp", cv32e40p_example_tb.PROGRAMS)
        self.assertEqual(
            "CV32E40P_HWLP_SIM_PASS",
            cv32e40p_example_tb.PROGRAMS["hwlp"]["pass_marker"],
        )
        netlist_command = hwlp_sim["commands"][0]
        sim_command = hwlp_sim["commands"][1]
        self.assertIn("regress/sv/helloworld_sim/cv32e40p_variant_netlist.py",
                      netlist_command[1])
        self.assertIn("--variant", netlist_command)
        self.assertIn("pulp", netlist_command)
        self.assertIn("{artifacts}/cv32e40p_pulp_naja.v", netlist_command)
        self.assertIn("{artifacts}/cv32e40p_pulp_naja.v", sim_command)
        self.assertIn("regress/sv/helloworld_sim/cv32e40p_example_tb.py", sim_command[1])
        self.assertIn("--program", sim_command)
        self.assertIn("hwlp", sim_command)
        self.assertIn(
            [
                "riscv32-unknown-elf-gcc",
                "riscv-none-elf-gcc",
                "riscv64-unknown-elf-gcc",
            ],
            hwlp_sim["tool_checks"],
        )

    def test_ibex_extended_sim_stages_use_upstream_simple_system_tests(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        ibex = sv_regress.select_cases(cases, "ibex")[0]

        self.assertIn("ibex_dit_sim", sv_regress.VALID_STAGES)
        self.assertIn("ibex_dummy_instr_sim", sv_regress.VALID_STAGES)
        self.assertIn("ibex_pmp_sim", sv_regress.VALID_STAGES)
        self.assertIn("ibex_secure_dit_sim", sv_regress.VALID_STAGES)
        self.assertIn("ibex_secure_dummy_instr_sim", sv_regress.VALID_STAGES)
        self.assertEqual("IBEX_DIT_SIM_PASS", ibex["ibex_dit_sim"]["pass_regex"])
        self.assertEqual(
            "IBEX_DUMMY_INSTR_SIM_PASS",
            ibex["ibex_dummy_instr_sim"]["pass_regex"],
        )
        self.assertIn("dit_test", ibex_simple_system.PROGRAMS)
        self.assertIn("dummy_instr_test", ibex_simple_system.PROGRAMS)
        self.assertIn("pmp_smoke_test", ibex_simple_system.PROGRAMS)
        self.assertEqual(
            "PASS: All test sequences behaved as expected",
            ibex_simple_system.PROGRAMS["dit_test"]["expected_output"],
        )
        for stage_name, program, marker in (
            ("ibex_dit_sim", "dit_test", "IBEX_DIT_SIM_PASS"),
            ("ibex_dummy_instr_sim", "dummy_instr_test", "IBEX_DUMMY_INSTR_SIM_PASS"),
        ):
            stage = ibex[stage_name]
            self.assertEqual(marker, stage["pass_regex"])
            self.assertNotIn("expected_failure", stage)
            self.assertNotIn("expected_failure_reason", stage)
            self.assertEqual(2, len(stage["commands"]))
            secure_command = stage["commands"][0]
            sim_command = stage["commands"][1]
            self.assertIn("regress/sv/helloworld_sim/ibex_secure_netlist.py", secure_command[1])
            self.assertIn("--output", secure_command)
            self.assertIn("{artifacts}/ibex_secure_naja.v", secure_command)
            self.assertIn("regress/sv/helloworld_sim/ibex_simple_system.py", sim_command[1])
            self.assertIn("{artifacts}/ibex_secure_naja.v", sim_command)
            self.assertIn("--secure-ibex", sim_command)
            self.assertIn("--program", sim_command)
            self.assertIn(program, sim_command)
            self.assertIn("--max-cycles", sim_command)
            self.assertIn(
                [
                    "riscv32-unknown-elf-gcc",
                    "riscv-none-elf-gcc",
                    "riscv64-unknown-elf-gcc",
                ],
                stage["tool_checks"],
            )

        pmp_stage = ibex["ibex_pmp_sim"]
        self.assertEqual("IBEX_PMP_SIM_PASS", pmp_stage["pass_regex"])
        self.assertNotIn("expected_failure", pmp_stage)
        self.assertEqual(2, len(pmp_stage["commands"]))
        self.assertEqual(
            "MCAUSE: 0x00000007",
            ibex_simple_system.PROGRAMS["pmp_smoke_test"]["expected_output"],
        )
        pmp_netlist_command = pmp_stage["commands"][0]
        pmp_sim_command = pmp_stage["commands"][1]
        self.assertIn("regress/sv/helloworld_sim/ibex_secure_netlist.py", pmp_netlist_command[1])
        self.assertIn("--variant", pmp_netlist_command)
        self.assertIn("pmp", pmp_netlist_command)
        self.assertIn("{artifacts}/ibex_pmp_naja.v", pmp_netlist_command)
        self.assertIn("{artifacts}/ibex_pmp_naja.v", pmp_sim_command)
        self.assertIn("pmp_smoke_test", pmp_sim_command)

        self.assertEqual("IBEX_SECURE_NETLIST_PASS", ibex_secure_netlist.SECURE_PASS_MARKER)
        for stage_name, program, marker in (
            ("ibex_secure_dit_sim", "dit_test", "IBEX_DIT_SIM_PASS"),
            (
                "ibex_secure_dummy_instr_sim",
                "dummy_instr_test",
                "IBEX_DUMMY_INSTR_SIM_PASS",
            ),
        ):
            stage = ibex[stage_name]
            self.assertEqual(marker, stage["pass_regex"])
            self.assertNotIn("expected_failure", stage)
            self.assertNotIn("expected_failure_reason", stage)
            self.assertEqual(2, len(stage["commands"]))
            secure_command = stage["commands"][0]
            sim_command = stage["commands"][1]
            self.assertIn("regress/sv/helloworld_sim/ibex_secure_netlist.py", secure_command[1])
            self.assertIn("--output", secure_command)
            self.assertIn("{artifacts}/ibex_secure_naja.v", secure_command)
            self.assertIn("regress/sv/helloworld_sim/ibex_simple_system.py", sim_command[1])
            self.assertIn("{artifacts}/ibex_secure_naja.v", sim_command)
            self.assertIn("--secure-ibex", sim_command)
            self.assertIn("--program", sim_command)
            self.assertIn(program, sim_command)

    def test_ibex_generated_netlist_check_accepts_current_generated_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            generated = Path(tmpdir) / "ibex_naja.v"
            generated.write_text("module generated; endmodule\n", encoding="utf-8")

            ibex_simple_system.check_generated_netlist(generated)

    def test_ibex_generated_netlist_check_rejects_old_generated_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            generated = Path(tmpdir) / "ibex_naja.v"
            generated.write_text("module generated; endmodule\n", encoding="utf-8")
            constructor_source = (
                ibex_simple_system.REPO_ROOT / "src" / "nl" / "formats" /
                "systemverilog" / "frontend" / "SNLSVConstructor.cpp"
            )
            old_time = constructor_source.stat().st_mtime - 1
            os.utime(generated, (old_time, old_time))

            with self.assertRaises(SystemExit) as context:
                ibex_simple_system.check_generated_netlist(generated)

        self.assertIn("generated netlist is older than", str(context.exception))

    def test_ibex_secure_netlist_flist_enables_secure_ibex(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)
            vc_path = tmpdir_path / "ibex.vc"
            vc_path.write_text(
                """
# ignored
+define+SYNTHESIS
+incdir+rtl/include
rtl/ibex_top.sv
""",
                encoding="utf-8",
            )

            content = ibex_secure_netlist.secure_flist_content(vc_path)

        self.assertTrue(content.startswith("-GSecureIbex=1\n"))
        self.assertIn("+define+SYNTHESIS", content)
        self.assertIn("+incdir+" + str(tmpdir_path / "rtl" / "include"), content)
        self.assertIn(str(tmpdir_path / "rtl" / "ibex_top.sv"), content)

    def test_ibex_pmp_netlist_flist_enables_pmp(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir_path = Path(tmpdir)
            vc_path = tmpdir_path / "ibex.vc"
            vc_path.write_text("rtl/ibex_top.sv\n", encoding="utf-8")

            content = ibex_secure_netlist.variant_flist_content(
                vc_path,
                ibex_secure_netlist.VARIANTS["pmp"]["params"],
            )

        self.assertTrue(content.startswith("-GPMPEnable=1\n"))
        self.assertIn(str(tmpdir_path / "rtl" / "ibex_top.sv"), content)


class CVA6TestharnessTest(unittest.TestCase):
    def test_default_jobs_uses_num_jobs_or_bounded_cpu_count(self):
        original_num_jobs = os.environ.get("NUM_JOBS")
        original_cpu_count = cva6_testharness.os.cpu_count
        try:
            os.environ["NUM_JOBS"] = "7"
            self.assertEqual("7", cva6_testharness.default_jobs())

            os.environ.pop("NUM_JOBS", None)
            cva6_testharness.os.cpu_count = lambda: 8
            self.assertEqual("2", cva6_testharness.default_jobs())

            cva6_testharness.os.cpu_count = lambda: None
            self.assertEqual("1", cva6_testharness.default_jobs())
        finally:
            cva6_testharness.os.cpu_count = original_cpu_count
            if original_num_jobs is None:
                os.environ.pop("NUM_JOBS", None)
            else:
                os.environ["NUM_JOBS"] = original_num_jobs

    def test_compile_program_links_baremetal_allocator_for_libgcc_emutls(self):
        calls = []
        original_run = cva6_testharness.run
        try:
            cva6_testharness.run = lambda command, **kwargs: calls.append((command, kwargs))
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                artifacts = tmpdir_path / "artifacts"
                elf = cva6_testharness.compile_program(
                    tmpdir_path / "repo",
                    artifacts,
                    "riscv-none-elf-",
                    "hello_world",
                )
                alloc_shim = artifacts / "cva6_baremetal_alloc.c"
                alloc_shim_content = alloc_shim.read_text(encoding="utf-8")

            self.assertEqual(1, len(calls))
            command = calls[0][0]
            self.assertEqual("riscv-none-elf-gcc", command[0])
            self.assertIn(str(alloc_shim), command)
            self.assertIn("void *malloc(size_t size)", alloc_shim_content)
            self.assertIn("void free(void *ptr)", alloc_shim_content)
            self.assertIn("-nostdlib", command)
            self.assertIn("-lgcc", command)
            self.assertTrue(str(elf).endswith("cva6_hello_world.o"))
        finally:
            cva6_testharness.run = original_run

    def test_build_verilator_model_uses_ccache_when_available(self):
        commands = []
        original_run = cva6_testharness.run
        original_which = cva6_testharness.shutil.which
        original_namespace = cva6_testharness.namespace_generated_netlist
        original_write_fesvr = cva6_testharness.write_fesvr_dpi_bridge
        original_write_stub = cva6_testharness.write_remote_bitbang_stub
        original_write_tb = cva6_testharness.write_patched_ariane_tb
        original_build_source_lists = cva6_testharness.build_source_lists
        try:
            cva6_testharness.run = lambda command, **kwargs: commands.append((command, kwargs))
            cva6_testharness.shutil.which = lambda tool: "/usr/bin/ccache" if tool == "ccache" else None
            cva6_testharness.namespace_generated_netlist = (
                lambda generated, stage_dir: generated
            )
            cva6_testharness.write_fesvr_dpi_bridge = (
                lambda repo, stage_dir: stage_dir / "naja_fesvr_dpi.cc"
            )
            cva6_testharness.write_remote_bitbang_stub = (
                lambda stage_dir: stage_dir / "naja_remote_bitbang_stub.cc"
            )
            cva6_testharness.write_patched_ariane_tb = (
                lambda repo, stage_dir: stage_dir / "ariane_tb_naja.cpp"
            )
            cva6_testharness.build_source_lists = (
                lambda repo, stage_dir, generated, primitives: ([], [generated, primitives])
            )
            with tempfile.TemporaryDirectory() as tmpdir:
                tmpdir_path = Path(tmpdir)
                repo = tmpdir_path / "repo"
                artifacts = tmpdir_path / "artifacts"
                spike_dir = tmpdir_path / "spike"
                verilator_dir = tmpdir_path / "verilator"
                generated = artifacts / "cva6.v"
                primitives = artifacts / "najaeda_primitives.v"
                (verilator_dir / "share" / "verilator" / "include").mkdir(parents=True)
                repo.mkdir()
                artifacts.mkdir()
                generated.write_text("module cva6; endmodule\n", encoding="utf-8")
                primitives.write_text("module primitive; endmodule\n", encoding="utf-8")

                executable = cva6_testharness.build_verilator_model(
                    repo,
                    artifacts,
                    generated,
                    primitives,
                    spike_dir,
                    verilator_dir,
                    "2",
                )

            self.assertTrue(str(executable).endswith("work-ver/Variane_testharness"))
            self.assertEqual(2, len(commands))
            make_command = commands[1][0]
            self.assertIn("-j2", make_command)
            self.assertIn("CXX=ccache c++", make_command)
        finally:
            cva6_testharness.run = original_run
            cva6_testharness.shutil.which = original_which
            cva6_testharness.namespace_generated_netlist = original_namespace
            cva6_testharness.write_fesvr_dpi_bridge = original_write_fesvr
            cva6_testharness.write_remote_bitbang_stub = original_write_stub
            cva6_testharness.write_patched_ariane_tb = original_write_tb
            cva6_testharness.build_source_lists = original_build_source_lists

    def test_run_sim_sets_linux_and_macos_spike_library_paths(self):
        calls = []
        original_run = cva6_testharness.run
        original_ld = os.environ.get("LD_LIBRARY_PATH")
        original_dyld = os.environ.get("DYLD_LIBRARY_PATH")
        try:
            cva6_testharness.run = lambda command, **kwargs: calls.append((command, kwargs))
            os.environ["LD_LIBRARY_PATH"] = "/existing/linux"
            os.environ["DYLD_LIBRARY_PATH"] = "/existing/macos"

            cva6_testharness.run_sim(
                Path("/tmp/cva6_helloworld_sim/work-ver/Variane_testharness"),
                Path("/tmp/fw.o"),
                "0000000080001000",
                Path("/opt/spike"),
                1234,
                ["+extra"],
            )

            self.assertEqual(1, len(calls))
            env = calls[0][1]["env"]
            self.assertEqual(
                f"/opt/spike/lib{os.pathsep}/existing/linux",
                env["LD_LIBRARY_PATH"],
            )
            self.assertEqual(
                f"/opt/spike/lib{os.pathsep}/existing/macos",
                env["DYLD_LIBRARY_PATH"],
            )
            self.assertIn("+extra", calls[0][0])
        finally:
            cva6_testharness.run = original_run
            if original_ld is None:
                os.environ.pop("LD_LIBRARY_PATH", None)
            else:
                os.environ["LD_LIBRARY_PATH"] = original_ld
            if original_dyld is None:
                os.environ.pop("DYLD_LIBRARY_PATH", None)
            else:
                os.environ["DYLD_LIBRARY_PATH"] = original_dyld


class CV32E40PExampleTbTest(unittest.TestCase):
    def test_manifest_parser_substitutes_design_rtl_dir(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            repo_dir = Path(tmpdir)
            rtl_dir = repo_dir / "rtl"
            rtl_dir.mkdir()
            (repo_dir / "cv32e40p_manifest.flist").write_text(
                """
// comment
+incdir+${DESIGN_RTL_DIR}/include
${DESIGN_RTL_DIR}/cv32e40p_pkg.sv

${DESIGN_RTL_DIR}/cv32e40p_top.sv
""",
                encoding="utf-8",
            )

            include_dirs, sources = cv32e40p_example_tb.read_cv32e40p_manifest(repo_dir)

        self.assertEqual([f"+incdir+{rtl_dir}/include"], include_dirs)
        self.assertEqual(
            [
                (rtl_dir / "cv32e40p_pkg.sv").resolve(),
                (rtl_dir / "cv32e40p_top.sv").resolve(),
            ],
            sources,
        )

    def test_hwlp_instruction_encoding_uses_corev_pulp_custom_opcode(self):
        setupi = cv32e40p_example_tb.hwlp_setupi_word("x1", "10", "16")
        self.assertEqual(0x00a246ab, setupi)

        setup = cv32e40p_example_tb.hwlp_setup_word("x1", "t0", "24")
        self.assertEqual(0x0062c7ab, setup)

    def test_cv32e40p_pulp_netlist_flist_enables_corev_pulp(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            repo_dir = Path(tmpdir)
            rtl_dir = repo_dir / "rtl"
            rtl_dir.mkdir()
            flist_path = repo_dir / "cv32e40p_manifest.flist"
            flist_path.write_text(
                """
// comment
+incdir+${DESIGN_RTL_DIR}/include
${DESIGN_RTL_DIR}/cv32e40p_top.sv
""",
                encoding="utf-8",
            )

            content = cv32e40p_variant_netlist.variant_flist_content(
                flist_path,
                cv32e40p_variant_netlist.VARIANTS["pulp"]["params"],
                rtl_dir=rtl_dir,
            )

        self.assertTrue(content.startswith("-GCOREV_PULP=1\n"))
        self.assertIn("+incdir+" + str(rtl_dir / "include"), content)
        self.assertIn(str(rtl_dir / "cv32e40p_top.sv"), content)


if __name__ == "__main__":
    unittest.main()
