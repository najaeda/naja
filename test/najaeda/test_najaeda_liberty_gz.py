# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest

from najaeda import netlist

liberty_benchmarks = os.environ.get('LIBERTY_BENCHMARKS_PATH')


class NajaNetlistLibertyGzTest(unittest.TestCase):
    def tearDown(self):
        netlist.reset()

    def test_load_liberty_gz(self):
        primitives = [os.path.join(liberty_benchmarks, "tests", "small.lib.gz")]
        netlist.load_liberty(primitives)
        top = netlist.create_top("top")
        and2_ins = top.create_child_instance("and2", "and2_ins")
        self.assertIsNotNone(and2_ins)
