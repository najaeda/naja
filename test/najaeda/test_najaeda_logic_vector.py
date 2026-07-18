# SPDX-FileCopyrightText: 2026 The Naja authors
#
# SPDX-License-Identifier: Apache-2.0

import unittest

from najaeda import netlist


class LogicVectorTest(unittest.TestCase):
    def test_four_state_access_and_formatting(self):
        value = netlist.LogicVector("8'b10XZ_0011")
        self.assertEqual(8, len(value))
        self.assertEqual("8'b10xz0011", str(value))
        self.assertEqual(netlist.LogicValue.ONE, value[0])
        self.assertEqual(netlist.LogicValue.Z, value[4])
        self.assertEqual(netlist.LogicValue.X, value[5])
        self.assertTrue(value.contains_x())
        self.assertTrue(value.contains_z())
        self.assertFalse(value.is_fully_known())
        with self.assertRaises(ValueError):
            int(value)

    def test_known_integer_conversion(self):
        value = netlist.LogicVector("4'b1010")
        self.assertTrue(value.is_fully_known())
        self.assertEqual(10, int(value))
        self.assertEqual(value, netlist.LogicVector(str(value)))

    def test_invalid_literal(self):
        for literal in (
            "", "0'b", "4'hf", "3'b10", "2'b0q",
            "4'b_1010", "4'b10__10", "4'b1010_",
        ):
            with self.subTest(literal=literal):
                with self.assertRaises(ValueError):
                    netlist.LogicVector(literal)


if __name__ == "__main__":
    unittest.main()
