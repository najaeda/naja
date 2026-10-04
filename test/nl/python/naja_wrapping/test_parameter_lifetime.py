# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import gc
import unittest

import naja


class ParameterLifetimeTests(unittest.TestCase):
    def setUp(self):
        self.universe = naja.NLUniverse.create()
        library = naja.NLLibrary.create(naja.NLDB.create(self.universe))
        self.model = naja.SNLDesign.create(library, "model")
        self.parameter = naja.SNLParameter.create_string(self.model, "INIT", "x")
        self.top = naja.SNLDesign.create(library, "top")
        self.instance = naja.SNLInstance.create(self.top, self.model, "i")
        self.inst_parameter = naja.SNLInstParameter.create(
            self.instance, self.parameter, "y")

    def tearDown(self):
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()

    def test_universe_destruction_invalidates_retained_parameter_wrappers(self):
        self.universe.destroy()
        with self.assertRaisesRegex(RuntimeError, "unbound"):
            self.parameter.getName()
        with self.assertRaisesRegex(RuntimeError, "unbound"):
            self.inst_parameter.getValue()
        # Used to dereference freed objects and abort from the deallocator.
        self.parameter = self.inst_parameter = None
        gc.collect()

    def test_instance_destruction_invalidates_only_its_parameter_wrapper(self):
        self.instance.destroy()
        with self.assertRaisesRegex(RuntimeError, "unbound"):
            self.inst_parameter.getValue()
        self.assertEqual("INIT", self.parameter.getName())
        self.inst_parameter = None
        gc.collect()

    def test_design_destruction_invalidates_parameter_wrapper(self):
        self.instance.destroy()
        self.model.destroy()
        with self.assertRaisesRegex(RuntimeError, "unbound"):
            self.parameter.getName()

    def test_explicit_parameter_destruction_invalidates_wrapper(self):
        self.instance.destroy()
        self.parameter.destroy()
        with self.assertRaisesRegex(RuntimeError, "unbound"):
            self.parameter.getName()


if __name__ == "__main__":
    unittest.main()
