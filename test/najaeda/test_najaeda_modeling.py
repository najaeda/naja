# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest
from najaeda import netlist

# Get the PYTHONPATH environment variable
pythonpath = os.environ.get('PYTHONPATH')

if pythonpath:
    # os.pathsep is the separator used in PYTHONPATH (':' on Unix, ';' on Windows)
    paths = pythonpath.split(os.pathsep)
    print("PYTHONPATH contains the following directories:")
    for path in paths:
        print(path)
else:
    print("PYTHONPATH is not set.")

liberty_benchmarks = os.environ.get('LIBERTY_BENCHMARKS_PATH')

class NajaNetlistModeling(unittest.TestCase):
    def setUp(self):
        primitives = [os.path.join(liberty_benchmarks, "tests" , "small.lib")]
        netlist.load_liberty(primitives)

    def tearDown(self):
        netlist.reset()
    
    def test(self):
        top = netlist.create_top('top')
        i0 = top.create_term('i0', netlist.Term.Direction.INPUT)
        i1 = top.create_term('i1', netlist.Term.Direction.INPUT)
        o = top.create_term('o', netlist.Term.Direction.OUTPUT)
        ck = top.create_term('ck', netlist.Term.Direction.INPUT)
        and2_ins = top.create_child_instance('and2', 'and2_ins')
        ff_ins = top.create_child_instance('FF', 'ff_ins')
        i0_net = top.create_net('i0')
        i1_net = top.create_net('i1')
        o_net = top.create_net('o')
        ck_net = top.create_net('ck')
        and2_z_net = top.create_net('and2_z')
        i0.connect_lower_net(i0_net)
        i1.connect_lower_net(i1_net)
        o.connect_lower_net(o_net)
        ck.connect_lower_net(ck_net)
        and2_ins.get_term('I0').connect_upper_net(i0_net)
        and2_ins.get_term('I1').connect_upper_net(i1_net)
        and2_ins.get_term('Z').connect_upper_net(and2_z_net)
        ff_ins.get_term('D').connect_upper_net(and2_z_net)
        ff_ins.get_term('CK').connect_upper_net(ck_net)
        ff_ins.get_term('Q').connect_upper_net(o_net)

        ck_input_deps = ff_ins.get_term('CK').get_clock_related_inputs()
        self.assertEqual(1, len(ck_input_deps))
        self.assertEqual(ck_input_deps[0], ff_ins.get_term('D'))

if __name__ == '__main__':
    faulthandler.enable()
    unittest.main()
