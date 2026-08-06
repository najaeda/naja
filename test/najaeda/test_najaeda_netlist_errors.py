
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler
import os
import tempfile

from najaeda import netlist
from najaeda import naja

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

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
