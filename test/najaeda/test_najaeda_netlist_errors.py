
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler
import os
import tempfile
import warnings
from pathlib import Path

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_gate_level_verilog_names_and_compatibility(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input.v"
            source.write_text("module top(input a, output y); assign y = a; endmodule\n")
            with warnings.catch_warnings(record=True) as caught:
                warnings.simplefilter("error", FutureWarning)
                config = netlist.GateLevelVerilogConfig(keep_assigns=False)
                dump_config = netlist.GateLevelVerilogDumpConfig(
                    dumpAssignsAsInstances=True)
                top = netlist.load_gate_level_verilog(str(source), config=config)
                self.assertEqual(top.get_name(), "top")
                top.dump_gate_level_verilog(
                    str(Path(directory) / "new.v"), config=dump_config)
            self.assertEqual(caught, [])
            with self.assertWarnsRegex(
                    FutureWarning, "VerilogConfig will be deprecated soon; use GateLevelVerilogConfig"):
                old_config = netlist.VerilogConfig(keep_assigns=False)
            self.assertIsInstance(old_config, netlist.GateLevelVerilogConfig)
            with self.assertWarnsRegex(
                    FutureWarning, "VerilogDumpConfig will be deprecated soon; use GateLevelVerilogDumpConfig"):
                old_dump_config = netlist.VerilogDumpConfig(dumpAssignsAsInstances=True)
            self.assertIsInstance(old_dump_config, netlist.GateLevelVerilogDumpConfig)
            # New calls also accept existing config objects without another warning.
            with warnings.catch_warnings():
                warnings.simplefilter("error", FutureWarning)
                top.dump_gate_level_verilog(
                    str(Path(directory) / "mixed.v"), old_dump_config)
            netlist.reset()
            with warnings.catch_warnings(record=True) as caught:
                warnings.simplefilter("always", FutureWarning)
                top = netlist.load_verilog(str(source), config=old_config)
                top.dump_verilog(str(Path(directory) / "old.v"), dump_config)
            self.assertEqual(len(caught), 2)
            self.assertIn("load_gate_level_verilog", str(caught[0].message))
            self.assertIn("dump_gate_level_verilog", str(caught[1].message))
            self.assertEqual(top.get_name(), "top")
            # Compare the actual netlist bodies, ignoring only timestamp banners.
            def body(name):
                text = (Path(directory) / name).read_text()
                return text[text.index("module "):]
            self.assertEqual(body("new.v"), body("old.v"))
            self.assertEqual(body("new.v"), body("mixed.v"))

    def test_gate_level_verilog_validation(self):
        with self.assertRaisesRegex(TypeError, "GateLevelVerilogConfig.keep_assigns"):
            netlist.GateLevelVerilogConfig(keep_assigns=1)
        with self.assertRaisesRegex(TypeError, "load_gate_level_verilog config"):
            netlist.load_gate_level_verilog([], config="invalid")
        with self.assertRaises(ValueError):
            netlist.GateLevelVerilogDumpConfig(rtlInfoDumpMode="invalid")
        with warnings.catch_warnings():
            warnings.simplefilter("error", FutureWarning)
            with self.assertRaisesRegex(FutureWarning, "load_verilog will be deprecated soon"):
                netlist.load_verilog([])

    def test_missing_model(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        self.assertRaises(Exception, top.create_child_instance, 'Module0', 'mod')

    def test_missing_verilog(self):
        self.assertRaises(TypeError, netlist.load_verilog)
        with self.assertRaisesRegex(
                ValueError, "No Verilog input files were provided"):
            netlist.load_verilog([])
        self.assertRaises(TypeError, netlist.load_system_verilog)
        with self.assertRaisesRegex(
                ValueError,
                "pass at least one file or set SystemVerilogConfig.flist"):
            netlist.load_system_verilog([])

    def test_loader_input_errors_identify_argument_and_path(self):
        with self.assertRaisesRegex(
                TypeError, r"Verilog files\[1\].*got int"):
            netlist.load_verilog(["valid.v", 3])

        missing = os.path.join(
            tempfile.gettempdir(), "najaeda-file-that-does-not-exist.v")
        with self.assertRaises(FileNotFoundError) as context:
            netlist.load_verilog(missing)
        message = str(context.exception)
        self.assertIn(repr(missing), message)
        self.assertIn("files[0]", message)
        self.assertIn("resolved to", message)

        with self.assertRaisesRegex(
                TypeError, r"Liberty files must be a path string or list.*got int"):
            netlist.load_liberty(7)

    def test_verilog_config_validation_errors(self):
        cases = [
            (
                {"keep_assigns": 1},
                TypeError,
                r"VerilogConfig\.keep_assigns must be a bool \(got int\)",
            ),
            (
                {"conflicting_design_name_policy": 1},
                TypeError,
                r"conflicting_design_name_policy must be a str \(got int\)",
            ),
            (
                {"conflicting_design_name_policy": "merge"},
                ValueError,
                r"Invalid conflicting_design_name_policy: 'merge'",
            ),
            (
                {"allow_unknown_designs": "true"},
                TypeError,
                r"allow_unknown_designs must be a bool or None \(got str\)",
            ),
        ]
        for kwargs, exception_type, message in cases:
            with self.subTest(kwargs=kwargs):
                with self.assertRaisesRegex(exception_type, message):
                    netlist.VerilogConfig(**kwargs)

    def test_loader_config_and_path_validation_errors(self):
        with self.assertRaisesRegex(
                TypeError,
                r"SystemVerilogConfig\.diagnostics_report_path "
                r"must be a path string or None \(got int\)"):
            netlist.SystemVerilogConfig(diagnostics_report_path=1)

        with self.assertRaisesRegex(
                ValueError, r"Verilog files\[0\] must not be empty"):
            netlist.load_verilog([" "])

        with self.assertRaisesRegex(
                TypeError,
                r"load_verilog config must be a VerilogConfig or None \(got str\)"):
            netlist.load_verilog([], config="invalid")

        with self.assertRaisesRegex(
                TypeError,
                r"load_system_verilog config must be a "
                r"SystemVerilogConfig or None \(got str\)"):
            netlist.load_system_verilog([], config="invalid")

        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(
                    ValueError, r"Verilog input path is not a file"):
                netlist.load_verilog(directory)

            config = netlist.SystemVerilogConfig(flist=directory)
            with self.assertRaisesRegex(
                    ValueError, r"SystemVerilogConfig\.flist is not a file"):
                netlist.load_system_verilog([], config=config)

    def test_width_mismatch(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        topTerm = top.create_input_term('Top')
        topNet = top.create_bus_net('net', 1, 0)
        self.assertRaises(Exception, topTerm.connect_upper_net, topNet)

    def test_empty_liberty(self):
        with self.assertRaisesRegex(
                ValueError, "No Liberty input files were provided"):
            netlist.load_liberty([])
            
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
