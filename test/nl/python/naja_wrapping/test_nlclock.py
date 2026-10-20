# SPDX-FileCopyrightText: 2026 The Naja authors
# SPDX-License-Identifier: Apache-2.0

import unittest

import naja


class NLClockTests(unittest.TestCase):
    def setUp(self):
        self.universe = naja.NLUniverse.create()
        self.db = naja.NLDB.create(self.universe)
        library = naja.NLLibrary.create(self.db)
        self.top = naja.SNLDesign.create(library, "top")
        self.clk = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Input, "clk")
        self.clk2 = naja.SNLScalarTerm.create(self.top, naja.SNLTerm.Direction.Input, "clk2")
        self.model = naja.SNLDesign.create(library, "model")
        self.model_clk = naja.SNLScalarTerm.create(self.model, naja.SNLTerm.Direction.Input, "clk")
        self.instance = naja.SNLInstance.create(self.top, self.model, "inst")

    def tearDown(self):
        if naja.NLUniverse.get() is not None:
            naja.NLUniverse.get().destroy()

    def test_primary_clock(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        self.assertEqual("sys", clock.getName())
        self.assertEqual("primary", clock.getKind())
        self.assertEqual(self.top.getName(), clock.getDesign().getName())
        self.assertEqual(10.0, clock.getPeriod())
        self.assertAlmostEqual(0.1, clock.getFrequency())
        self.assertEqual("ns", clock.getTimeUnit())
        self.assertEqual(0.0, clock.getRiseAt())
        self.assertEqual(5.0, clock.getFallAt())
        self.assertIsNone(clock.getMaster())
        self.assertIsNone(clock.getMasterSource())
        self.assertEqual(1, clock.getDivideBy())
        self.assertEqual(1, clock.getMultiplyBy())
        self.assertFalse(clock.isInverted())
        self.assertEqual("sys", clock.getRootClock().getName())
        self.assertTrue(clock.isSynchronousWith(clock))
        sources = clock.getSources()
        self.assertEqual(1, len(sources))
        self.assertEqual("clk", sources[0].getName())

    def test_set_waveform(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        clock.setWaveform(2.0, 7.0)
        self.assertEqual(2.0, clock.getRiseAt())
        self.assertEqual(7.0, clock.getFallAt())
        with self.assertRaises(RuntimeError):
            clock.setWaveform(5.0, 2.0)
        self.assertEqual(7.0, clock.getFallAt())

    def test_generated_clock(self):
        master = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        divided = naja.NLClock.createGenerated(
            self.top, "div2", master, self.clk, 2, 1, False, [self.clk2])
        self.assertEqual("generated", divided.getKind())
        self.assertEqual(master.getName(), divided.getMaster().getName())
        self.assertEqual("clk", divided.getMasterSource().getName())
        self.assertEqual(20.0, divided.getPeriod())
        self.assertAlmostEqual(0.05, divided.getFrequency())
        self.assertEqual(2, divided.getDivideBy())
        self.assertEqual("sys", divided.getRootClock().getName())
        self.assertTrue(divided.isSynchronousWith(master))

    def test_unrelated_clocks_are_not_synchronous(self):
        sys_clock = naja.NLClock.createPrimary(self.top, "sys", 20.0, "ns", [self.clk])
        other = naja.NLClock.createPrimary(self.top, "other", 20.0, "ns", [self.clk2])
        self.assertFalse(sys_clock.isSynchronousWith(other))

    def test_multiple_sources(self):
        clock = naja.NLClock.createPrimary(
            self.top, "sys", 10.0, "ns", [self.clk, self.clk2])
        self.assertEqual([self.clk, self.clk2], clock.getSources())

    def test_primary_binding_errors(self):
        valid = (self.top, "sys", 10.0, "ns", [self.clk])
        cases = [
            ((), "malformed NLClock primary creation method"),
            ((self.top, "sys", "bad", "ns", [self.clk]),
             "malformed NLClock primary creation method"),
            ((None, *valid[1:]), "accepts SNLDesign as first argument"),
            ((*valid[:4], [self.clk, None]), "sources must be SNL terminals"),
        ]
        for args, message in cases:
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, message):
                    naja.NLClock.createPrimary(*args)
                self.assertEqual([], list(self.top.getClocks()))

    def test_generated_binding_errors(self):
        master = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        valid = (self.top, "derived", master, self.clk, 2, 1, False, [self.clk2])
        cases = [
            ((), "malformed NLClock generated creation method"),
            ((*valid[:4], "bad", *valid[5:]),
             "malformed NLClock generated creation method"),
            ((None, *valid[1:]), "accepts SNLDesign as first argument"),
            ((*valid[:2], None, *valid[3:]), "expects a master NLClock"),
            ((*valid[:3], None, *valid[4:]), "expects an SNL terminal as master source"),
            ((*valid[:7], (self.clk2,)), "sources must be a list of terminals"),
            ((*valid[:7], [self.clk2, None]), "sources must be SNL terminals"),
        ]
        for args, message in cases:
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, message):
                    naja.NLClock.createGenerated(*args)
                self.assertEqual([master], list(self.top.getClocks()))

    def test_waveform_binding_errors(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        for args in ((), (1.0,), (1.0, 2.0, 3.0), ("bad", 2.0)):
            with self.subTest(args=args):
                with self.assertRaisesRegex(RuntimeError, "expects \\(rise_at, fall_at\\)"):
                    clock.setWaveform(*args)
                self.assertEqual((0.0, 5.0), (clock.getRiseAt(), clock.getFallAt()))

    def test_synchronous_binding_errors(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        for other in (None, self.clk, "sys"):
            with self.subTest(other=other):
                with self.assertRaisesRegex(RuntimeError, "expects an NLClock"):
                    clock.isSynchronousWith(other)

    def test_rejects_instance_terminal_source(self):
        inst_term = self.instance.getInstTerm(self.model_clk)
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [inst_term])

    def test_rejects_invalid_arguments(self):
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 0.0, "ns", [self.clk])
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [])
        with self.assertRaises(RuntimeError):
            naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", "clk")

    def test_design_and_db_expose_clocks(self):
        sys_clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        design_clocks = list(self.top.getClocks())
        self.assertEqual(1, len(design_clocks))
        self.assertEqual("sys", design_clocks[0].getName())
        db_clocks = list(self.db.getClocks())
        self.assertEqual(1, len(db_clocks))
        self.assertEqual(sys_clock.getName(), db_clocks[0].getName())

    def test_destroy_removes_clock(self):
        clock = naja.NLClock.createPrimary(self.top, "sys", 10.0, "ns", [self.clk])
        clock.destroy()
        self.assertEqual(0, sum(1 for _ in self.top.getClocks()))
        self.assertEqual(0, sum(1 for _ in self.db.getClocks()))


if __name__ == "__main__":
    unittest.main()
