# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import unittest

from najaeda import netlist
from najaeda import naja
import logging

najaeda_source_test_path = os.environ.get('NAJAEDA_SOURCE_TEST_PATH')

class NajaNetlistCustomPrimitivesTest(unittest.TestCase):
    def setUp(self):
        logging.basicConfig(level=logging.DEBUG)
        universe = naja.NLUniverse.create()
        db = naja.NLDB.create(universe)
        universe.setTopDB(db)

    def tearDown(self):
        if naja.NLUniverse.get():
            naja.NLUniverse.get().destroy()

    def test_loading(self):
        universe = naja.NLUniverse.get()
        self.assertIsNotNone(universe)
        db = universe.getTopDB()
        self.assertIsNotNone(db)
        primitives_path = os.path.join(najaeda_source_test_path, 'test_najaeda_custom_primitives', 'primitives.py')
        netlist.load_primitives_from_file(primitives_path)

        primitives = db.getLibrary("custom_lib")
        self.assertIsNotNone(primitives)
        nb_primitives = sum(1 for _ in primitives.getSNLDesigns())
        self.assertEqual(nb_primitives, 2)
        self.assertIsNotNone(primitives.getSNLDesign("AND2"))
        self.assertIsNotNone(primitives.getSNLDesign("OR2"))
        self.assertIsNotNone(primitives.getSNLDesign("AND2").getScalarTerm("I0"))
        self.assertIsNotNone(primitives.getSNLDesign("AND2").getScalarTerm("I1"))
        self.assertIsNotNone(primitives.getSNLDesign("AND2").getScalarTerm("O"))
        self.assertIsNotNone(primitives.getSNLDesign("OR2").getScalarTerm("I0"))
        self.assertIsNotNone(primitives.getSNLDesign("OR2").getScalarTerm("I1"))
        self.assertIsNotNone(primitives.getSNLDesign("OR2").getScalarTerm("O"))

        #now instantiate in najaeda
        top = netlist.create_top('Top')
        self.assertIsNotNone(top)
        and2_ins0 = top.create_child_instance('AND2', 'and2_ins0')
        self.assertIsNotNone(and2_ins0)
        or2_ins0 = top.create_child_instance('OR2', 'or2_ins0')
        self.assertIsNotNone(or2_ins0)
        net = top.create_net('net')
        self.assertIsNotNone(net)
        and2_ins0.get_term("O").connect(net)
        or2_ins0.get_term("I0").connect(net)
        nb_connections = sum(1 for _ in net.get_inst_terms())
        self.assertEqual(nb_connections, 2)

    def test_errors(self):
        universe = naja.NLUniverse.get()
        self.assertIsNotNone(universe)
        db = universe.getTopDB()
        self.assertIsNotNone(db)
        self.assertRaises(Exception, netlist.load_primitives_from_file, "non_existent_file.py")
        primitives_path = os.path.join(najaeda_source_test_path, 'test_najaeda_custom_primitives', 'error_primitives.py')
        self.assertRaises(Exception, netlist.load_primitives_from_file, primitives_path)