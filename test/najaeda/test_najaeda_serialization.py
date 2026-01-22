# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import unittest
from najaeda.remote.serialization import (
    direction_to_int,
    serialize_design_ref,
    serialize_model,
    serialize_term,
    serialize_equipotential_term,
    serialize_equipotential_occurrence,
)
from najaeda import naja

class TestSerialization(unittest.TestCase):

    def setUp(self):
        u = naja.NLUniverse.create()
        db = naja.NLDB.create(u)
        self.lib = naja.NLLibrary.create(db, "TestLib")
        self.design = naja.SNLDesign.create(self.lib, "TestDesign")

    def tearDown(self):
        naja.NLUniverse.get().destroy()

    # ------------------------------------------------------------
    # direction_to_int
    # ------------------------------------------------------------
    def test_direction_to_int(self):
        self.assertEqual(direction_to_int(naja.SNLTerm.Direction.Input), 0)
        self.assertEqual(direction_to_int(naja.SNLTerm.Direction.Output), 1)
        self.assertEqual(direction_to_int(naja.SNLTerm.Direction.InOut), 2)

    # ------------------------------------------------------------
    # serialize_design_ref
    # ------------------------------------------------------------
    def test_serialize_design_ref(self):
        ref = serialize_design_ref(self.design)
        self.assertEqual(ref["db_id"], self.design.getNLID()[0])
        self.assertEqual(ref["library_id"], self.design.getNLID()[1])
        self.assertEqual(ref["design_id"], self.design.getNLID()[2])

    # ------------------------------------------------------------
    # serialize_model
    # ------------------------------------------------------------
    def test_serialize_model(self):
        payload = serialize_model(self.design, child_id=7, name="TOP")
        self.assertEqual(payload["name"], "TOP")
        self.assertEqual(payload["child_id"], 7)
        self.assertEqual(payload["model_name"], "TestDesign")
        self.assertIn("design_ref", payload)
        self.assertIn("has_terms", payload)
        self.assertIn("has_primitives", payload)
        self.assertIn("has_instances", payload)

    # ------------------------------------------------------------
    # serialize_term (scalar + bus + bus bit)
    # ------------------------------------------------------------
    def test_serialize_scalar_term(self):
        term = naja.SNLScalarTerm.create(
            self.design,
            naja.SNLTerm.Direction.Input,
            "A"
        )
        p = serialize_term(term)
        self.assertEqual(p["name"], "A")
        self.assertEqual(p["direction"], 0)
        self.assertIsNone(p["msb"])
        self.assertIsNone(p["lsb"])
        self.assertIsNone(p["bit"])

    def test_serialize_bus_term(self):
        bus = naja.SNLBusTerm.create(
            self.design,
            naja.SNLTerm.Direction.Output,
            7, 0,
            "BUS"
        )
        p = serialize_term(bus)
        self.assertEqual(p["msb"], 7)
        self.assertEqual(p["lsb"], 0)
        self.assertIsNone(p["bit"])

    def test_serialize_bus_term_bit(self):
        bus = naja.SNLBusTerm.create(
            self.design,
            naja.SNLTerm.Direction.Output,
            3, 0,
            "BUS"
        )
        bit2 = bus.getBusTermBit(2)
        p = serialize_term(bit2)
        self.assertEqual(p["bit"], 2)

    # ------------------------------------------------------------
    # serialize_equipotential_term
    # ------------------------------------------------------------
    def test_serialize_equipotential_term(self):
        term = naja.SNLScalarTerm.create(
            self.design,
            naja.SNLTerm.Direction.Input,
            "CLK"
        )
        p = serialize_equipotential_term(term)
        self.assertEqual(p["name"], "CLK")
        self.assertEqual(p["direction"], 0)

    # ------------------------------------------------------------
    # serialize_equipotential_occurrence
    # ------------------------------------------------------------
    def test_serialize_equipotential_occurrence(self):
        mid = naja.SNLDesign.create(self.lib, "Mid")
        leaf = naja.SNLDesign.create(self.lib, "Leaf")
        leaf_in = naja.SNLScalarTerm.create(
            leaf,
            naja.SNLTerm.Direction.Input,
            "IN"
        )

        inst1 = naja.SNLInstance.create(self.design, mid, "inst1")
        inst2 = naja.SNLInstance.create(mid, leaf, "inst2")

        path0 = naja.SNLPath()
        path1 = naja.SNLPath(path0, inst1)
        inst_terms = tuple(inst2.getInstTerms())
        inst_term = inst_terms[0]
        occ = naja.SNLOccurrence(path1, inst_term)

        payload = serialize_equipotential_occurrence(occ)
        self.assertEqual(
            payload["path"],
            [
                [inst1.getName(), inst1.getID()],
                [inst2.getName(), inst2.getID()],
            ],
        )
        self.assertEqual(payload["term_id"], leaf_in.getID())
        self.assertEqual(payload["name"], leaf_in.getName())
        self.assertEqual(payload["direction"], direction_to_int(leaf_in.getDirection()))
        self.assertIsNone(payload["bit"])


if __name__ == "__main__":
    unittest.main()
