
# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
import faulthandler

from najaeda import netlist
from najaeda import snl

class NajaNetlistTestErrors(unittest.TestCase):
    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()

    def test_missing_model(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        self.assertRaises(Exception, top.create_child_instance, 'Module0', 'mod')

    def test_width_mismatch(self):
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        topTerm = top.create_input_term('Top')
        topNet = top.create_net('net')
        self.assertRaises(Exception, topTerm.connect(topNet))
            
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()