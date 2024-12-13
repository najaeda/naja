
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import snl

class NajaNetlistTestXilinx(unittest.TestCase):
    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_xilinx_primitives(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        netlist.load_primitives('xilinx')

        lut2_ins = top.create_child_instance('LUT2', 'ins0')
        self.assertIsNotNone(lut2_ins)

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()