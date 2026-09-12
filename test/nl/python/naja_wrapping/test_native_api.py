# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import ctypes
import subprocess
import sys
import unittest

import naja
import naja_native_api_consumer


class NajaNativeAPITest(unittest.TestCase):
  def test_build_info_schema(self):
    info = naja.naja_build_info()
    self.assertEqual("najaeda.naja", info["provider"])
    self.assertEqual(1, info["api_version"])
    self.assertEqual(naja.getVersion(), info["naja_version"])
    self.assertEqual(naja.getGitHash(), info["git_hash"])
    self.assertTrue(info["build_id"].startswith("naja-native-v1:"))
    self.assertEqual("shared", info["runtime_kind"])
    self.assertNotIn("runtime_identity", info)

  def test_private_capsule_name(self):
    capsule_is_valid = ctypes.pythonapi.PyCapsule_IsValid
    capsule_is_valid.argtypes = [ctypes.py_object, ctypes.c_char_p]
    capsule_is_valid.restype = ctypes.c_int
    self.assertEqual(
      1,
      capsule_is_valid(naja._C_API, b"najaeda.naja._C_API"))
    self.assertEqual(
      0,
      capsule_is_valid(naja._C_API, b"foreign._C_API"))

  def test_external_consumer_round_trip(self):
    universe = naja.NLUniverse.create()
    try:
      db = naja.NLDB.create(universe)
      library = naja.NLLibrary.create(db, "work")
      design = naja.SNLDesign.create(library, "top")
      term = naja.SNLScalarTerm.create(
        design, naja.SNLTerm.Direction.Input, "clock")

      self.assertIs(
        design,
        naja_native_api_consumer.round_trip_design(design))
      self.assertIs(
        term,
        naja_native_api_consumer.round_trip_object(term))

      with self.assertRaises(TypeError):
        naja_native_api_consumer.round_trip_design(term)
      with self.assertRaises(TypeError):
        naja_native_api_consumer.round_trip_object(object())
    finally:
      universe.destroy()

  def test_consumer_first_import_order(self):
    subprocess.run(
      [
        sys.executable,
        "-c",
        "import naja_native_api_consumer; import naja",
      ],
      check=True)


if __name__ == "__main__":
  unittest.main()
