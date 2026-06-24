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
      "library": library,
      "top": top,
      "scalar_net": scalar_net,
      "bus_net": bus_net,
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

  def assertNLIDConstructorError(self, fields, message):
    with self.assertRaises(RuntimeError) as context:
      naja.NLID(*fields)
    self.assertEqual(message, str(context.exception))

  def test_constructor_rejects_invalid_fields(self):
    valid = [
      naja.NLID.Type.Instance,
      1,
      2,
      3,
      4,
      5,
      6,
    ]

    with self.assertRaises(RuntimeError) as context:
      naja.NLID(*valid[:-1])
    self.assertEqual("NLID constructor expects seven integer arguments", str(context.exception))

    self.assertNLIDConstructorError(
      ["instance", *valid[1:]],
      "NLID type must be an integer")
    self.assertNLIDConstructorError(
      [255, *valid[1:]],
      "NLID type is out of range")
    self.assertNLIDConstructorError(
      [*valid[:1], "db", *valid[2:]],
      "NLID dbID must be an integer")
    self.assertNLIDConstructorError(
      [*valid[:1], 2**32, *valid[2:]],
      "NLID dbID is out of range")
    self.assertNLIDConstructorError(
      [*valid[:2], 2**32, *valid[3:]],
      "NLID libraryID is out of range")
    self.assertNLIDConstructorError(
      [*valid[:3], 2**32, *valid[4:]],
      "NLID designID is out of range")
    self.assertNLIDConstructorError(
      [*valid[:4], 2**32, *valid[5:]],
      "NLID designObjectID is out of range")
    self.assertNLIDConstructorError(
      [*valid[:5], 2**32, valid[6]],
      "NLID instanceID is out of range")
    self.assertNLIDConstructorError(
      [*valid[:6], "bit"],
      "NLID bit must be an integer")
    self.assertNLIDConstructorError(
      [*valid[:6], 2**31],
      "NLID bit is out of range")

    with self.assertRaises(OverflowError):
      naja.NLID(*[*valid[:6], 2**100])
    with self.assertRaises(OverflowError):
      naja.NLID(*[*valid[:1], -1, *valid[2:]])

  def test_from_string_rejects_invalid_input(self):
    with self.assertRaises(RuntimeError) as context:
      naja.NLID.from_string(1)
    self.assertEqual("NLID.from_string expects a string", str(context.exception))

    for value in ("", "not-an-nlid", "NLID(1:2:3:4:5:6:7"):
      with self.subTest(value=value):
        with self.assertRaises(RuntimeError) as context:
          naja.NLID.from_string(value)
        self.assertEqual(
          "NLID.from_string expects NLID(t:db:lib:design:object:instance:bit)",
          str(context.exception))

    for value in ("NLID(1:2:3:4:5:6)", "NLID(1:2:3:4:5:6:7:8)", "NLID(1,2:3:4:5:6:7)"):
      with self.subTest(value=value):
        with self.assertRaises(RuntimeError) as context:
          naja.NLID.from_string(value)
        self.assertEqual(
          "NLID.from_string expects NLID(t:db:lib:design:object:instance:bit)",
          str(context.exception))

  def test_value_semantics_and_get_object(self):
    objects = self.create_design()
    universe = naja.NLUniverse.get()

    ids = {name: obj.getNLID() for name, obj in objects.items()}
    self.assertTrue(all(isinstance(nlid, naja.NLID) for nlid in ids.values()))
    self.assertEqual(len(ids), len(set(ids.values())))

    self.assertEqual("label", {ids["instance"]: "label"}[objects["instance"].getNLID()])
    self.assertEqual(ids["instance"], objects["instance"].getNLID())
    self.assertNotEqual(ids["instance"], ids["scalar_net"])
    self.assertNotEqual(ids["instance"], object())
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
    self.assertEqual(0, ids["instance"].getInstanceID())
    self.assertFalse(ids["scalar_net"].isInstance())
    self.assertFalse(ids["instance"].isNet())
    self.assertFalse(ids["scalar_net"].isTerm())
    self.assertFalse(ids["instance"].isDesign())

    for name, obj in objects.items():
      resolved = universe.getObject(ids[name])
      if name == "bus_net":
        self.assertIsNone(resolved)
      else:
        self.assertEqual(obj, resolved)

    with self.assertRaises(RuntimeError) as context:
      universe.getObject("not an NLID")
    self.assertEqual("NLUniverse.getObject expects an NLID argument", str(context.exception))

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
