# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
from najaeda import naja


def constructAND(lib):
    and2 = naja.SNLDesign.createPrimitive(lib, "$_AND_")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Output, "Y")


def constructOR(lib):
    or2 = naja.SNLDesign.createPrimitive(lib, "$_OR_")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Output, "Y")


def constructXOR(lib):
    xor2 = naja.SNLDesign.createPrimitive(lib, "$_XOR_")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Output, "Y")


def constructDFFP(lib):
    dffp = naja.SNLDesign.createPrimitive(lib, "$_DFF_P_")
    naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Output, "Q")


def load(db):
    logging.info("Loading Yosys primitives")
    lib = naja.NLLibrary.createPrimitives(db, "yosys")
    constructAND(lib)
    constructOR(lib)
    constructXOR(lib)
    constructDFFP(lib)
