# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest
import faulthandler

from najaeda import netlist, naja

systemverilog_benchmarks = os.environ.get("SYSTEMVERILOG_BENCHMARKS_PATH")
if not systemverilog_benchmarks:
    verilog_benchmarks = os.environ.get("VERILOG_BENCHMARKS_PATH")
    if verilog_benchmarks:
        systemverilog_benchmarks = verilog_benchmarks.replace(
            "/verilog/benchmarks", "/systemverilog/benchmarks")
najaeda_test_path = os.environ.get("NAJAEDA_TEST_PATH")
if not najaeda_test_path:
    najaeda_test_path = os.getcwd()


class NajaEDASystemVerilogTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_unsupported_multi_writer_memory_blackboxing(self):
        # Exercise both Python API levels, independent clocks and a shared clock.
        for raw in (False, True):
            for clock_b in ("clk_b", "clk_a"):
                for mode in ("strict", "fallback", "define"):
                    with self.subTest(raw=raw, clock_b=clock_b, mode=mode):
                        netlist.reset()
                        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
                            source = os.path.join(directory, "dp_ram.sv")
                            report = os.path.join(directory, "diagnostics.log")
                            with open(source, "w") as stream:
                                stream.write("""// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module dp_ram(input logic clk_a, clk_b, we_a, we_b,
              input logic [2:0] addr_a, addr_b,
              input logic [7:0] wd_a, wd_b,
              output logic [7:0] rd_a, rd_b);
`ifndef SYNTHESIS_MEMORY_BLACK_BOXING
  logic [7:0] mem [8];
  always @(posedge clk_a) begin
    if (we_a) mem[addr_a] = wd_a;
    rd_a <= mem[addr_a];
  end
  always @(posedge CLOCK_B) begin
    if (we_b) mem[addr_b] = wd_b;
    rd_b <= mem[addr_b];
  end
`endif
endmodule
module top(input logic clk_a, clk_b, we_a, we_b,
           input logic [2:0] addr_a, addr_b,
           input logic [7:0] wd_a, wd_b,
           output logic [7:0] rd_a, rd_b, output logic alive);
  dp_ram ram(.*);
  assign alive = we_a & we_b;
endmodule
""".replace("CLOCK_B", clock_b))
                            options = dict(
                                diagnostics_report_path=report,
                                blackbox_multi_writer_memories=(mode == "fallback"),
                                defines=(["SYNTHESIS_MEMORY_BLACK_BOXING"]
                                         if mode == "define" else []))
                            if raw:
                                db = naja.NLDB.create(naja.NLUniverse.create())
                                load = lambda: db.loadSystemVerilog([source], **options)
                            else:
                                load = lambda: netlist.load_system_verilog(
                                    source, netlist.SystemVerilogConfig(**options))
                            if mode == "strict":
                                with self.assertRaisesRegex(
                                        RuntimeError, "multiple sequential writers"):
                                    load()
                                continue
                            load()
                            top = naja.NLUniverse.get().getTopDesign()
                            self.assertFalse(top.isBlackBox())
                            ram = top.getInstance("ram")
                            self.assertIsNotNone(ram)
                            model = ram.getModel()
                            self.assertTrue(model.isBlackBox())
                            self.assertEqual(0, len(list(model.getInstances())))
                            self.assertEqual(0 if mode == "fallback" else 10,
                                             len(list(model.getNets())))
                            self.assertEqual(10, len(list(model.getTerms())))
                            for name, width in (("addr_a", 3), ("addr_b", 3),
                                                ("wd_a", 8), ("wd_b", 8),
                                                ("rd_a", 8), ("rd_b", 8)):
                                term = model.getBusTerm(name)
                                self.assertEqual(width, term.getWidth())
                                self.assertEqual(naja.SNLTerm.Direction.Output
                                                 if name.startswith("rd")
                                                 else naja.SNLTerm.Direction.Input,
                                                 term.getDirection())
                            for term in ram.getInstTerms():
                                self.assertIsNotNone(term.getNet())
                            self.assertGreater(len(list(top.getInstances())), 1)
                            with open(report) as stream:
                                diagnostics = stream.read()
                            if mode == "fallback":
                                self.assertIn("multi_writer_memory_blackbox", diagnostics)
                                self.assertIn("Memory 'mem'", diagnostics)
                                self.assertIn("blackboxing entire module 'dp_ram'", diagnostics)
                            else:
                                self.assertNotIn("multi_writer_memory_blackbox", diagnostics)

    def test_multi_clock_memory_inference_and_snapshot(self):
        for raw in (False, True):
            for clock_b in ("clk_b", "clk_a"):
                with self.subTest(raw=raw, clock_b=clock_b):
                    netlist.reset()
                    with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
                        source = os.path.join(directory, "dp_ram.sv")
                        with open(source, "w") as stream:
                            stream.write("""// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module dp_ram(input logic clk_a, clk_b, we_a, we_b,
              input logic [2:0] addr_a, addr_b,
              input logic [7:0] wd_a, wd_b,
              output logic [7:0] rd_a, rd_b);
  logic [7:0] mem [8];
  always @(posedge clk_a) begin
    if (we_a) mem[addr_a] <= wd_a;
    rd_a <= mem[addr_a];
  end
  always @(posedge CLOCK_B) begin
    if (we_b) mem[addr_b] <= wd_b;
    rd_b <= mem[addr_b];
  end
endmodule
""".replace("CLOCK_B", clock_b))
                        report = os.path.join(directory, "diagnostics.log")
                        if raw:
                            db = naja.NLDB.create(naja.NLUniverse.create())
                            db.loadSystemVerilog([source], diagnostics_report_path=report)
                        else:
                            netlist.load_system_verilog(source, netlist.SystemVerilogConfig(
                                diagnostics_report_path=report))
                        top = naja.NLUniverse.get().getTopDesign()
                        self.assertFalse(top.isBlackBox())
                        snapshot = os.path.join(directory, "snapshot")
                        for restored in (False, True):
                            if restored:
                                top.getLibrary().getDB().dumpNajaIF(snapshot)
                                netlist.reset()
                                naja.NLDB.loadNajaIF(snapshot)
                                top = naja.NLUniverse.get().getTopDesign()
                            memories = [inst for inst in top.getInstances()
                                        if inst.getModel().getName().startswith("naja_mem__")]
                            self.assertEqual(1, len(memories))
                            memory = memories[0]
                            model = memory.getModel()
                            self.assertEqual("1", model.getParameter("MULTI_CLOCK").getValue())
                            self.assertEqual(2, model.getBusTerm("WCLK").getWidth())
                            self.assertEqual(16, model.getBusTerm("WMASK").getWidth())
                            for port in range(2):
                                clock = model.getBusTerm("WCLK").getBusTermBit(port)
                                self.assertTrue(clock.isClock())
                                self.assertEqual(20, len(list(
                                    naja.SNLDesign.getClockRelatedInputs(clock))))
                                self.assertEqual(16, len(list(
                                    naja.SNLDesign.getClockRelatedOutputs(clock))))
                                for bit in range(8):
                                    mask = model.getBusTerm("WMASK").getBusTermBit(port * 8 + bit)
                                    self.assertEqual(naja.SNLTermRole.MemoryWriteEnable,
                                                     mask.getRole())
                            for term in memory.getInstTerms():
                                self.assertIsNotNone(term.getNet())
                            self.assertIsNone(top.getNet("mem"))
                        with open(report) as stream:
                            diagnostics = stream.read()
                        self.assertIn("multi_clock_memory_collision", diagnostics)
                        self.assertNotIn("multi_writer_memory_blackbox", diagnostics)

    def test_multi_writer_memory_option_validation(self):
        self.assertFalse(netlist.SystemVerilogConfig().blackbox_multi_writer_memories)
        with self.assertRaisesRegex(TypeError, "blackbox_multi_writer_memories"):
            netlist.SystemVerilogConfig(blackbox_multi_writer_memories="true")

    def test_memory_sync_reset_role(self):
        for condition, level in (("rst", naja.SNLActiveLevel.High),
                                 ("~rst", naja.SNLActiveLevel.Low)):
            with self.subTest(condition=condition):
                netlist.reset()
                with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
                    source = os.path.join(temp_dir, "memory.sv")
                    with open(source, "w") as stream:
                        stream.write("""// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
module memory(input logic clk, rst,
              input logic [1:0] addr, input logic [7:0] data,
              output logic [7:0] q);
  logic [7:0] mem_q [0:3];
  logic [7:0] mem_d [0:3];
  always_comb begin
    mem_d = mem_q;
    mem_d[addr] = data;
  end
  always_ff @(posedge clk) begin
    if (CONDITION) mem_q <= '{8'h10, 8'h21, 8'h32, 8'h43};
    else mem_q <= mem_d;
  end
  assign q = mem_q[addr];
endmodule
""".replace("CONDITION", condition))
                    netlist.load_system_verilog(source)
                    top = naja.NLUniverse.get().getTopDesign()
                    memories = [inst.getModel() for inst in top.getInstances()
                                if inst.getModel().getName().startswith("naja_mem__")]
                    self.assertEqual(1, len(memories))
                    reset = memories[0].getScalarTerm("RST")
                    self.assertIsNotNone(reset)
                    self.assertEqual(naja.SNLTermRole.SyncReset, reset.getRole())
                    self.assertTrue(reset.isReset())
                    self.assertTrue(reset.isSyncReset())
                    self.assertFalse(reset.isAsyncReset())
                    self.assertEqual(level, reset.getResetActiveLevel())

    def test_system_verilog_config_diagnostics_default(self):
        config = netlist.SystemVerilogConfig()
        self.assertEqual(
            "naja_sv_diagnostics.log", config.diagnostics_report_path)

    def test_load_system_verilog(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())
        self.assertEqual(3, top.count_terms())
        self.assertEqual(2, top.count_input_terms())
        self.assertEqual(1, top.count_output_terms())
        source_range = top.get_source_range()
        self.assertIsInstance(source_range, netlist.SourceRange)
        self.assertTrue(source_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))
        self.assertGreaterEqual(source_range.line, 1)
        self.assertGreaterEqual(source_range.column, 1)
        self.assertGreaterEqual(source_range.end_line, source_range.line)
        self.assertGreaterEqual(source_range.end_column, 1)

        term_range = top.get_term("a").get_source_range()
        self.assertIsInstance(term_range, netlist.SourceRange)
        self.assertTrue(term_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

        net_range = top.get_net("y").get_source_range()
        self.assertIsInstance(net_range, netlist.SourceRange)
        self.assertTrue(net_range.file.endswith(
            "systemverilog/benchmarks/simple/simple.sv"))

    def test_load_system_verilog_single_arg(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        top = netlist.load_system_verilog(design_file)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_with_ast_link_option(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        top = netlist.load_system_verilog(
            design_file,
            config=netlist.SystemVerilogConfig(keep_ast_link=True),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_source_range_absent(self):
        top = netlist.create_top("top")
        self.assertIsNone(top.get_source_range())

        concat_net = netlist.Net([], net_concat=[])
        self.assertIsNone(concat_net.get_source_range())

    def test_load_system_verilog_with_ast_json_dump(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        json_path = os.path.join(najaeda_test_path, "simple_elaborated_ast_najaeda.json")
        diagnostics_path = os.path.join(najaeda_test_path, "simple_diagnostics_najaeda.txt")
        top = netlist.load_system_verilog(
            design_files,
            config=netlist.SystemVerilogConfig(
                elaborated_ast_json_path=json_path,
                diagnostics_report_path=diagnostics_path),
        )
        self.assertIsNotNone(top)
        self.assertTrue(os.path.exists(json_path))
        self.assertTrue(os.path.exists(diagnostics_path))

    def test_dump_verilog_with_config(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)

        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as dump_dir:
            without_rtl_infos = os.path.join(dump_dir, "simple_default_no_rtl_infos.v")
            top.dump_verilog(without_rtl_infos)
            with open(without_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)

            with_rtl_infos = os.path.join(dump_dir, "simple_with_rtl_infos.v")
            top.dump_verilog(
                with_rtl_infos,
                config=netlist.VerilogDumpConfig(dumpRTLInfosAsAttributes=True),
            )
            with open(with_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertNotIn("sv_src_file", dumped_text)
            self.assertIn('naja_sv_src="', dumped_text)

            with_verbose_rtl_infos = os.path.join(
                dump_dir, "simple_with_verbose_rtl_infos.v"
            )
            top.dump_verilog(
                with_verbose_rtl_infos,
                config=netlist.VerilogDumpConfig(
                    dumpRTLInfosAsAttributes=True,
                    rtlInfoDumpMode="VerboseAttributes",
                ),
            )
            with open(with_verbose_rtl_infos, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("sv_src_file", dumped_text)
            self.assertNotIn('naja_sv_src="', dumped_text)

            with_assign_instances = os.path.join(dump_dir, "simple_with_assign_instances.v")
            top.dump_verilog(
                with_assign_instances,
                config=netlist.VerilogDumpConfig(dumpAssignsAsInstances=True),
            )
            with open(with_assign_instances, "r", encoding="utf-8") as dumped_file:
                dumped_text = dumped_file.read()
            self.assertIn("assign_module", dumped_text)

    def test_dump_verilog_split_packed_signals(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as directory:
            source = os.path.join(directory, "packed.sv")
            with open(source, "w", encoding="utf-8") as stream:
                stream.write("module child(input [3:0] a, output [3:0] y); "
                             "assign y = a; endmodule\n"
                             "module top(input [3:0] a, output [3:0] y); "
                             "child i(a,y); endmodule\n")
            top = netlist.load_system_verilog([source], config=netlist.SystemVerilogConfig(top="top"))
            path = os.path.join(directory, "split.v")
            top.dump_verilog(path, config=netlist.VerilogDumpConfig(verilatorSplitPackedSignals=True))
            with open(path, encoding="utf-8") as stream:
                text = stream.read()
            child_text, top_text = text.split("module top(")
            self.assertIn("split_var", child_text)
            self.assertNotIn("split_var", top_text.split(");", 1)[0])

    def test_dump_verilog_config_rejects_invalid_rtl_info_mode(self):
        with self.assertRaises(ValueError) as context:
            netlist.VerilogDumpConfig(rtlInfoDumpMode="Invalid")
        self.assertIn("Invalid rtlInfoDumpMode", str(context.exception))

    def test_load_system_verilog_with_flist(self):
        design_file = os.path.join(systemverilog_benchmarks, "simple", "simple.sv")
        flist_path = os.path.join(najaeda_test_path, "simple_najaeda.f")
        with open(flist_path, "w", encoding="utf-8") as flist:
            flist.write(f"{design_file}\n")
        top = netlist.load_system_verilog(
            [],
            config=netlist.SystemVerilogConfig(flist=flist_path),
        )
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_alias(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        top = netlist.load_system_verilog(design_files)
        self.assertIsNotNone(top)
        self.assertEqual("top", top.get_model_name())

    def test_load_system_verilog_with_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            top = netlist.load_system_verilog(
                [generic_sv, top_sv],
                config=netlist.SystemVerilogConfig(top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())

    def test_load_system_verilog_with_non_string_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.top must be a str or None \(got int\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=123),
            )

    def test_load_system_verilog_with_empty_top_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                ValueError,
                r"SystemVerilogConfig\.top must not be empty"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(top=""),
            )

    def test_load_system_verilog_with_invalid_ast_link_option_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.keep_ast_link must be a bool \(got str\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(keep_ast_link="true"),
            )

    def test_load_system_verilog_with_invalid_blackbox_unknown_modules_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.blackbox_unknown_modules "
                r"must be a bool \(got str\)"):
            netlist.load_system_verilog(
                design_files,
                config=netlist.SystemVerilogConfig(blackbox_unknown_modules="true"),
            )

    def test_load_system_verilog_with_invalid_defines_raises(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        cases = [
            (
                "SYNTHESIS",
                TypeError,
                r"SystemVerilogConfig\.defines must be a list\[str\] or None "
                r"\(got str\)",
            ),
            (
                [1],
                TypeError,
                r"SystemVerilogConfig\.defines\[0\] must be a str \(got int\)",
            ),
            (
                [""],
                ValueError,
                r"SystemVerilogConfig\.defines\[0\] must not be empty",
            ),
            (
                ["HAS SPACE"],
                ValueError,
                r"SystemVerilogConfig\.defines\[0\] must not contain whitespace",
            ),
        ]
        for defines, exception_type, message in cases:
            with self.subTest(defines=defines):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.load_system_verilog(
                        design_files,
                        config=netlist.SystemVerilogConfig(defines=defines),
                    )

    def test_load_system_verilog_configuration_errors_are_actionable(self):
        design_files = [os.path.join(systemverilog_benchmarks, "simple", "simple.sv")]
        cases = [
            (
                {"diagnostics_report_path": ""},
                ValueError,
                r"diagnostics_report_path must not be empty",
            ),
            (
                {"suppress_warnings": [1]},
                TypeError,
                r"suppress_warnings\[0\] must be a str \(got int\)",
            ),
            (
                {"suppress_warnings": ["-Wno-width-trunc"]},
                ValueError,
                r"without a -W/-Wno- prefix; got '-Wno-width-trunc'",
            ),
        ]
        for kwargs, exception_type, message in cases:
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.load_system_verilog(
                        design_files,
                        config=netlist.SystemVerilogConfig(**kwargs),
                    )

        missing_flist = os.path.join(
            najaeda_test_path, "missing-systemverilog-command-file.f")
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(flist=missing_flist),
            )
        self.assertIn("SystemVerilogConfig.flist", str(context.exception))
        self.assertIn(repr(missing_flist), str(context.exception))

    def test_load_system_verilog_with_flist_top_and_define(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            design_sv = os.path.join(temp_dir, "define_selects_top.sv")
            with open(design_sv, "w", encoding="utf-8") as design_file:
                design_file.write(
                    "`ifdef SYNTHESIS\n"
                    "module synth_top(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n"
                    "`else\n"
                    "module sim_top(input logic a, output logic y);\n"
                    "  assign y = ~a;\n"
                    "endmodule\n"
                    "`endif\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f"{design_sv}\n")

            top = netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="synth_top",
                    defines=["SYNTHESIS"]),
            )
            self.assertIsNotNone(top)
            self.assertEqual("synth_top", top.get_model_name())

    def test_load_system_verilog_with_flist_and_top(self):
        with tempfile.TemporaryDirectory(dir=najaeda_test_path) as temp_dir:
            generic_sv = os.path.join(temp_dir, "generic.sv")
            with open(generic_sv, "w", encoding="utf-8") as generic_file:
                generic_file.write(
                    "module generic #(parameter type T = logic) "
                    "(input T i, output logic y);\n"
                    "  assign y = i.foo;\n"
                    "endmodule\n")

            top_sv = os.path.join(temp_dir, "top2.sv")
            with open(top_sv, "w", encoding="utf-8") as top_file:
                top_file.write(
                    "module top2(input logic a, output logic y);\n"
                    "  assign y = a;\n"
                    "endmodule\n")

            source_flist = os.path.join(temp_dir, "sources.f")
            with open(source_flist, "w", encoding="utf-8") as flist:
                flist.write(f"{generic_sv}\n")
                flist.write(f"{top_sv}\n")

            top = netlist.load_system_verilog(
                [],
                config=netlist.SystemVerilogConfig(
                    flist=source_flist,
                    top="top2"),
            )
            self.assertIsNotNone(top)
            self.assertEqual("top2", top.get_model_name())


if __name__ == "__main__":
    faulthandler.enable()
    unittest.main()
