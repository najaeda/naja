# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import unittest
import warnings

import naja

# old name -> (callable taking the old name's arguments, new name)
DEPRECATED = {
    "snapshot_manifest": "snapshotManifest",
    "live_compilation": "liveCompilation",
    "ast_symbol_of": "astSymbolOf",
    "snl_objects_of": "snlObjectsOf",
    "intent_available": "intentAvailable",
    "intent_type_of": "intentTypeOf",
    "intent_parameters_of": "intentParametersOf",
    "intent_package_member": "intentPackageMember",
}

PARAMETER_DEPRECATED = {
    "create_string": "createString",
    "create_decimal": "createDecimal",
    "create_binary": "createBinary",
    "create_boolean": "createBoolean",
}


class DeprecatedNamesTests(unittest.TestCase):
    def setUp(self):
        self.universe = naja.NLUniverse.create()
        self.db = naja.NLDB.create(self.universe)
        self.library = naja.NLLibrary.create(self.db)
        self.design = naja.SNLDesign.create(self.library, "m")

    def tearDown(self):
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()

    def assertWarnsDeprecated(self, old, new, call):
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            try:
                call()
            except Exception:
                # The wrapped call may fail on purpose; only the warning matters here.
                pass
        messages = [str(w.message) for w in caught
                    if issubclass(w.category, DeprecationWarning)]
        self.assertIn(f"{old}() is deprecated and will be removed in next release, use {new}() instead", messages)

    def test_module_function_aliases_warn(self):
        # Arguments follow each function's method flags: none for NOARGS, one for METH_O.
        calls = {
            "snapshot_manifest": lambda: naja.snapshot_manifest("/nonexistent/snl.mf"),
            "live_compilation": lambda: naja.live_compilation(),
            "ast_symbol_of": lambda: naja.ast_symbol_of(self.design),
            "snl_objects_of": lambda: naja.snl_objects_of(self.design),
            "intent_available": lambda: naja.intent_available(),
            "intent_type_of": lambda: naja.intent_type_of(self.design),
            "intent_parameters_of": lambda: naja.intent_parameters_of(self.design),
            "intent_package_member": lambda: naja.intent_package_member("pkg", "member"),
        }
        self.assertEqual(set(DEPRECATED), set(calls))
        for old, new in DEPRECATED.items():
            with self.subTest(old=old):
                self.assertWarnsDeprecated(old, new, calls[old])

    def test_parameter_creator_aliases_warn_and_still_work(self):
        for old, new in PARAMETER_DEPRECATED.items():
            with self.subTest(old=old):
                self.assertWarnsDeprecated(
                    old, new,
                    lambda old=old: getattr(naja.SNLParameter, old)(self.design, "P", "v"))
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            parameter = naja.SNLParameter.create_string(self.design, "X", "value")
        self.assertEqual("value", parameter.getValue())

    def test_nlid_from_string_alias_warns(self):
        self.assertWarnsDeprecated("from_string", "fromString",
                                   lambda: naja.NLID.from_string("invalid"))

    def test_new_names_do_not_warn(self):
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            naja.intentAvailable()
            naja.SNLParameter.createString(self.design, "Y", "w")
        self.assertEqual([], [w for w in caught if issubclass(w.category, DeprecationWarning)])


if __name__ == "__main__":
    unittest.main()
