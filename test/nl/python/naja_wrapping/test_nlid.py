# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import os
import tempfile
import unittest

import naja


class NLIDTest(unittest.TestCase):
  def tearDown(self):
    if naja.NLUniverse.get():
      naja.NLUniverse.get().destroy()

  def create_design(self):
    universe = naja.NLUniverse.create()
    db = naja.NLDB.create(universe)
    library = naja.NLLibrary.create(db, "work")
    model = naja.SNLDesign.create(library, "model")
    model_input = naja.SNLScalarTerm.create(model, naja.SNLTerm.Direction.Input, "i")
    model_output = naja.SNLBusTerm.create(model, naja.SNLTerm.Direction.Output, 3, 0, "o")
    top = naja.SNLDesign.create(library, "top")
    scalar_net = naja.SNLScalarNet.create(top, "n")
    bus_net = naja.SNLBusNet.create(top, 3, 0, "b")
    instance = naja.SNLInstance.create(top, model, "u0")
    inst_term = instance.getInstTerm(model_input)
    universe.setTopDesign(top)
    return {
      "db": db,
      "top": top,
      "scalar_net": scalar_net,
      "bus_net_bit": bus_net.getBit(2),
      "model_input": model_input,
      "model_output_bit": model_output.getBusTermBit(1),
      "instance": instance,
      "inst_term": inst_term,
    }

  def assertNLIDRoundTrip(self, nlid):
    fields = nlid.toTuple()
    self.assertEqual(7, len(fields))
    self.assertEqual(nlid, naja.NLID(*fields))
    self.assertEqual(nlid, naja.NLID.from_string(str(nlid)))
    self.assertEqual(str(nlid), repr(nlid))

  def test_value_semantics_and_get_object(self):
    objects = self.create_design()
    universe = naja.NLUniverse.get()

    ids = {name: obj.getNLID() for name, obj in objects.items()}
    self.assertTrue(all(isinstance(nlid, naja.NLID) for nlid in ids.values()))
    self.assertEqual(len(ids), len(set(ids.values())))

    self.assertEqual("label", {ids["instance"]: "label"}[objects["instance"].getNLID()])
    self.assertEqual(ids["instance"], objects["instance"].getNLID())
    self.assertNotEqual(ids["instance"], ids["scalar_net"])
    self.assertTrue(min(ids.values()) < max(ids.values()))

    for nlid in ids.values():
      self.assertNLIDRoundTrip(nlid)

    self.assertTrue(ids["top"].isDesign())
    self.assertTrue(ids["instance"].isInstance())
    self.assertTrue(ids["scalar_net"].isNet())
    self.assertTrue(ids["bus_net_bit"].isNet())
    self.assertTrue(ids["model_input"].isTerm())
    self.assertTrue(ids["model_output_bit"].isTerm())
    self.assertTrue(ids["inst_term"].isTerm())
    self.assertEqual(naja.NLID.Type.Instance, ids["instance"].getType())

    for name, obj in objects.items():
      self.assertEqual(obj, universe.getObject(ids[name]))

    unknown = naja.NLID(
      naja.NLID.Type.Instance,
      ids["instance"].getDBID(),
      ids["instance"].getLibraryID(),
      ids["instance"].getDesignID(),
      ids["instance"].getDesignObjectID(),
      999,
      ids["instance"].getBit())
    self.assertIsNone(universe.getObject(unknown))

  def test_snapshot_round_trip_keeps_distinct_nlids(self):
    objects = self.create_design()
    db = objects["db"]
    before = {
      "instance:u0": objects["instance"].getNLID(),
      "net:n": objects["scalar_net"].getNLID(),
    }
    self.assertNotEqual(before["instance:u0"], before["net:n"])

    with tempfile.TemporaryDirectory() as dump_dir:
      naja_dir = os.path.join(dump_dir, "snapshot.naja")
      db.dumpNajaIF(naja_dir)
      naja.NLUniverse.get().destroy()

      naja.NLDB.loadNajaIF(naja_dir)
      universe = naja.NLUniverse.get()
      top = universe.getTopDesign()
      self.assertIsNotNone(top)
      after = {
        "instance:u0": top.getInstance("u0").getNLID(),
        "net:n": top.getScalarNet("n").getNLID(),
      }
      self.assertEqual(before, after)
      self.assertEqual(top.getInstance("u0"), universe.getObject(after["instance:u0"]))
      self.assertEqual(top.getScalarNet("n"), universe.getObject(after["net:n"]))


if __name__ == "__main__":
  unittest.main()
