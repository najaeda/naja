# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
import faulthandler
from najaeda import netlist
from najaeda import snl

class NajaNetlistTest1(unittest.TestCase):
    def setUp(self):
        ##create hierarchical netlist
        top = netlist.create_top('Top')
        #ins0 = top.create_instance('Ins0')

    def tearDown(self):
        if snl.SNLUniverse.get():
            snl.SNLUniverse.get().destroy()
    
    def test0(self):
        pass

    
if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()