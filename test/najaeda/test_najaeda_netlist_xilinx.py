
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
        i = top.create_input_term("I")
        o = top.create_output_term("O")
        self.assertIsNotNone(top)
        netlist.load_primitives('xilinx')

        lut2_ins0 = top.create_child_instance('LUT2', 'ins0')
        lut2_ins1 = top.create_child_instance('LUT2', 'ins1')
        self.assertIsNotNone(lut2_ins0)
        self.assertIsNotNone(lut2_ins1)

        net1 = top.create_net("net1") 
        net2 = top.create_net("net2")

        i.connect(net1)
        lut2_ins0.get_term("I0").connect(net1)
        lut2_ins0.get_term("O").connect(net2)
        lut2_ins1.get_term("I0").connect(net2)
        o.connect(net2)

        top.dump_full_dot('./test_xilinx_primitives.dot')
        with self.assertRaises(Exception) as context: top.dump_full_dot(-1)
        top.dump_context_dot('./test_xilinx_primitives_context.dot')
        with self.assertRaises(Exception) as context: top.dump_context_dot(-1)

        leaf_drivers_count = 0
        for leaf_driver in lut2_ins1.get_term("I0").get_equipotential().get_leaf_drivers():
            leaf_drivers_count += 1
        top_readers_count = 0
        for top_reader in lut2_ins1.get_term("I0").get_equipotential().get_top_readers():
            top_readers_count += 1
        top_drivers_count = 0
        for top_driver in lut2_ins0.get_term("I0").get_equipotential().get_top_drivers():
            top_drivers_count +=1
        
        self.assertEqual(i.get_equipotential(), lut2_ins0.get_term("I0").get_equipotential())

        self.assertEqual(1, leaf_drivers_count)
        self.assertEqual(1, top_readers_count)
        self.assertEqual(1, top_drivers_count)
        self.assertEqual(lut2_ins0.get_design(), top)

        leaf_count = 0
        for leaf in top.get_leaf_children():
            leaf_count += 1
        
        self.assertEqual(2, leaf_count)

        top.delete_instance_by_id(0)
        lut2_ins1.delete()


if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()