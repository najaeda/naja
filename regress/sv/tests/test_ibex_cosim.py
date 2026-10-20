# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import sys
import tempfile
import unittest

REGRESS_SV_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REGRESS_SV_ROOT))
sys.path.insert(0, str(REGRESS_SV_ROOT / "cases" / "ibex_cheriot"))

import ibex_cosim  # noqa: E402

try:
    import yaml  # noqa: F401
    import sv_regress
except ImportError:  # pragma: no cover - PyYAML is optional for these tests.
    sv_regress = None

PORTS = [
    ["clk_i", "INPUT", 1],
    ["rst_ni", "INPUT", 1],
    ["fetch_enable_i", "INPUT", 4],
    ["irq_nm_i", "INPUT", 1],
    ["instr_rdata_i", "INPUT", 32],
    ["wide_i", "INPUT", 70],
    ["instr_req_o", "OUTPUT", 1],
    ["data_req_o", "OUTPUT", 1],
    ["instr_addr_o", "OUTPUT", 32],
    ["wide_o", "OUTPUT", 70],
]


def write_vcd(path: Path, scope: str, changes: list[str]) -> None:
    path.write_text(
        "$timescale 1ps $end\n"
        "$scope module TOP $end\n"
        f"$scope module {scope} $end\n"
        "$var wire 1 ! data_req_o $end\n"
        "$var wire 4 \" state_q [3:0] $end\n"
        "$upscope $end\n$upscope $end\n$enddefinitions $end\n"
        + "\n".join(changes) + "\n",
        encoding="utf-8",
    )


class IbexCosimTest(unittest.TestCase):
    def test_repo_root_and_base_flist(self):
        self.assertEqual(REGRESS_SV_ROOT.parents[1], ibex_cosim.REPO_ROOT)
        text = ibex_cosim.BASE_FLIST.read_text(encoding="utf-8")
        self.assertIn("rtl/ibex_top.sv", text)
        for line in text.splitlines():
            # Sources resolve from the Ibex checkout root, never from a local path.
            if not line.startswith("//"):
                self.assertFalse(line.startswith("/"), line)

    def test_flists_use_slang_and_numeric_values(self):
        config = ibex_cosim.CONFIGS["opentitan"]
        slang = ibex_cosim.flist_text(config, verilator=False)
        verilator = ibex_cosim.flist_text(config, verilator=True)
        self.assertIn(f"-f {ibex_cosim.BASE_FLIST}", slang.splitlines()[0])
        self.assertIn("-G BaseIsa=ibex_pkg::BaseIsaRV32IorCHERIoT", slang)
        self.assertIn("-GBaseIsa=1", verilator)
        self.assertIn("-GRV32B=2", verilator)
        self.assertNotIn("ibex_pkg::", verilator)
        self.assertEqual(ibex_cosim.flist_text({}, verilator=True),
                         f"-f {ibex_cosim.BASE_FLIST}\n")

    def test_every_config_overrides_known_opentitan_parameters(self):
        for name, config in ibex_cosim.CONFIGS.items():
            for parameter, values in config.items():
                with self.subTest(config=name, parameter=parameter):
                    self.assertIn(parameter, ibex_cosim.OPENTITAN)
                    self.assertEqual(ibex_cosim.OPENTITAN[parameter], values)

    def test_harness_drives_inputs_and_compares_outputs(self):
        harness = ibex_cosim.generate_harness(PORTS, max_mismatches=7)
        self.assertIn("a->fetch_enable_i = b->fetch_enable_i = 0x5;", harness)
        self.assertIn("rare() ? rnd(1) : 0; a->irq_nm_i = v;", harness)
        self.assertIn("uint64_t v = rnd(32); a->instr_rdata_i = v;", harness)
        self.assertIn("a->wide_i[2] = v; b->wide_i[2] = v;", harness)
        self.assertNotIn("a->clk_i = b->clk_i = rnd", harness)
        self.assertIn('MISMATCH cyc=%llu %s instr_addr_o rtl=', harness)
        self.assertIn("a->wide_o[2] != b->wide_o[2]", harness)
        self.assertNotIn("a->fetch_enable_i != b->fetch_enable_i", harness)
        self.assertIn("if (mismatches > 7)", harness)
        self.assertIn('printf("RESULT seed=', harness)

    def test_normalized_dump_ignores_header(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            first, second = Path(tmpdir) / "a.v", Path(tmpdir) / "b.v"
            first.write_text("// Fri Oct 16 23:32:48 2026\nmodule t();\nendmodule\n")
            second.write_text("// Sat Oct 17 08:00:00 2026\nmodule t();\nendmodule\n")
            self.assertEqual(ibex_cosim.normalized_dump(first),
                             ibex_cosim.normalized_dump(second))
            second.write_text("// header\nmodule t(input a);\nendmodule\n")
            self.assertNotEqual(ibex_cosim.normalized_dump(first),
                                ibex_cosim.normalized_dump(second))

    def test_signal_names_match_across_rtl_and_netlist(self):
        rtl = ibex_cosim.normalize_signal(["ibex_top", "u_ibex_core", "if_stage_i", "fetch_q"])
        naja = ibex_cosim.normalize_signal(["ibex_top", "ibex_top_u_ibex_core_if_stage_i",
                                            "fetch_q"])
        self.assertEqual("ibex_top_u_ibex_core_if_stage_i_fetch_q", naja)
        self.assertTrue(rtl.endswith("u_ibex_core_if_stage_i_fetch_q"))
        self.assertEqual("ibex_top_x_q_3", ibex_cosim.normalize_signal(["ibex_top_x", "q[3]"]))

    def test_first_divergences_reports_earliest_signal(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            rtl, naja = Path(tmpdir) / "rtl.vcd", Path(tmpdir) / "naja.vcd"
            write_vcd(rtl, "ibex_top", ["#0", "0!", "b0 \"", "#10", "b11 \"", "#20", "1!"])
            write_vcd(naja, "ibex_top", ["#0", "0!", "b0 \"", "#10", "b11 \"", "#20", "0!"])
            divergences = ibex_cosim.first_divergences(rtl, naja)
            self.assertEqual(1, len(divergences))
            self.assertIn("t=20 (cycle ~2) ibex_top_data_req_o: rtl=1 naja=0", divergences[0])

    def test_rtl_cache_key_depends_on_configuration(self):
        class FakeTools:
            version = "Verilator 5.052"

        with tempfile.TemporaryDirectory() as tmpdir:
            key_default = ibex_cosim.rtl_cache_key(FakeTools(), Path(tmpdir), "-f base\n")
            key_pmp = ibex_cosim.rtl_cache_key(FakeTools(), Path(tmpdir), "-f base\n-GPMPEnable=1\n")
            self.assertNotEqual(key_default, key_pmp)
            self.assertEqual(key_default,
                             ibex_cosim.rtl_cache_key(FakeTools(), Path(tmpdir), "-f base\n"))

    @unittest.skipIf(sv_regress is None, "PyYAML is not installed")
    def test_manifest_stage(self):
        cases = sv_regress.load_manifest(sv_regress.DEFAULT_MANIFEST)
        case = sv_regress.select_cases(cases, "ibex_cheriot")[0]
        self.assertEqual("7a4697f578c67c550fcad5e5e3078415e1f088d7", case["commit"])
        self.assertEqual(["ibex_cosim"], sv_regress.select_stages(["ibex_cosim"], [case]))
        stage = case["ibex_cosim"]
        self.assertEqual(ibex_cosim.PASS_MARKER, stage["pass_regex"])
        command = stage["commands"][0]
        self.assertIn("{naja}/regress/sv/cases/ibex_cheriot/ibex_cosim.py", command)
        self.assertIn("--rtl-cache", command)
        flist = case["flist"].replace("{naja}", str(sv_regress.REPO_ROOT))
        self.assertTrue(Path(flist).is_file(), flist)


if __name__ == "__main__":
    unittest.main()
