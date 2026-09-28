# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import ctypes
import unittest

import naja


class RuntimeAPI(ctypes.Structure):
    _fields_ = [
        ("abi_version", ctypes.c_uint32), ("struct_size", ctypes.c_size_t),
        ("version", ctypes.c_char_p), ("git_commit", ctypes.c_char_p),
        ("compiler", ctypes.c_char_p), ("build_id", ctypes.c_char_p),
        ("runtime_identity", ctypes.c_void_p), ("get_universe", ctypes.c_void_p),
        ("unwrap_design", ctypes.c_void_p),
    ]


class RuntimeAPITests(unittest.TestCase):
    def setUp(self):
        get_pointer = ctypes.pythonapi.PyCapsule_GetPointer
        get_pointer.argtypes = [ctypes.py_object, ctypes.c_char_p]
        get_pointer.restype = ctypes.c_void_p
        self.api = RuntimeAPI.from_address(get_pointer(naja._C_API, b"najaeda.naja._C_API"))
        # PYFUNCTYPE retains the GIL and propagates errors raised by the API.
        self.unwrap = ctypes.PYFUNCTYPE(ctypes.c_void_p, ctypes.py_object)(self.api.unwrap_design)
        self.get_universe = ctypes.PYFUNCTYPE(ctypes.c_void_p)(self.api.get_universe)
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()
        self.addCleanup(self.cleanup_universe)

    @staticmethod
    def cleanup_universe():
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()

    def test_version_and_runtime_identity(self):
        self.assertEqual(1, self.api.abi_version)
        self.assertGreaterEqual(self.api.struct_size, ctypes.sizeof(RuntimeAPI))
        self.assertTrue(self.api.runtime_identity)
        self.assertTrue(self.api.version)
        self.assertIsNone(self.get_universe())
        naja.NLUniverse.create()
        self.assertTrue(self.get_universe())

    def test_checked_borrow_rejects_wrong_and_destroyed_objects(self):
        with self.assertRaises(TypeError):
            self.unwrap(object())
        universe = naja.NLUniverse.create()
        library = naja.NLLibrary.create(naja.NLDB.create(universe))
        design = naja.SNLDesign.create(library, "borrowed")
        pointer = self.unwrap(design)
        self.assertTrue(pointer)
        self.assertEqual(pointer, self.unwrap(design))
        design.destroy()
        with self.assertRaises(ReferenceError):
            self.unwrap(design)


if __name__ == "__main__":
    unittest.main()
