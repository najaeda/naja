# SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

import logging
from najaeda import naja
from . import utils


def constructAND(lib):
    and2 = naja.SNLDesign.createPrimitive(lib, "$_AND_")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Output, "Y")
    and2.setTruthTable(0x8)


def constructOR(lib):
    or2 = naja.SNLDesign.createPrimitive(lib, "$_OR_")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Output, "Y")
    or2.setTruthTable(0xE)


def constructXOR(lib):
    xor2 = naja.SNLDesign.createPrimitive(lib, "$_XOR_")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(xor2, naja.SNLTerm.Direction.Output, "Y")
    xor2.setTruthTable(0x6)


def constructXNOR(lib):
    xnor2 = naja.SNLDesign.createPrimitive(lib, "$_XNOR_")
    naja.SNLScalarTerm.create(xnor2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(xnor2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(xnor2, naja.SNLTerm.Direction.Output, "Y")
    xnor2.setTruthTable(0x9)


def constructMUX(lib):
    mux2 = naja.SNLDesign.createPrimitive(lib, "$_MUX_")
    naja.SNLScalarTerm.create(mux2, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(mux2, naja.SNLTerm.Direction.Input, "B")
    naja.SNLScalarTerm.create(mux2, naja.SNLTerm.Direction.Input, "S")
    naja.SNLScalarTerm.create(mux2, naja.SNLTerm.Direction.Output, "Y")
    mux2.setTruthTable(0xCA)


def constructNOT(lib):
    not_gate = naja.SNLDesign.createPrimitive(lib, "$_NOT_")
    naja.SNLScalarTerm.create(not_gate, naja.SNLTerm.Direction.Input, "A")
    naja.SNLScalarTerm.create(not_gate, naja.SNLTerm.Direction.Output, "Y")
    not_gate.setTruthTable(0b01)


def constructDFFP(lib):
    dffp = naja.SNLDesign.createPrimitive(lib, "$_DFF_P_")
    c = naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffp, naja.SNLTerm.Direction.Output, "Q")
    utils.constructSequentialPrimitive(dffp, c)


def constructDFFE_PP(lib):
    dffe_pp = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PP_")
    c = naja.SNLScalarTerm.create(dffe_pp, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pp, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pp, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pp, naja.SNLTerm.Direction.Output, "Q")
    utils.constructSequentialPrimitive(dffe_pp, c)


def constructDFFE_PN(lib):
    dffe_pn = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PN_")
    c = naja.SNLScalarTerm.create(dffe_pn, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pn, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pn, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pn, naja.SNLTerm.Direction.Output, "Q")
    utils.constructSequentialPrimitive(dffe_pn, c)


def constructDFF_PP0(lib):
    dff_pp0 = naja.SNLDesign.createPrimitive(lib, "$_DFF_PP0_")
    c = naja.SNLScalarTerm.create(dff_pp0, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dff_pp0, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dff_pp0, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dff_pp0, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dff_pp0, c)


def constructDFF_PN0(lib):
    dff_pn0 = naja.SNLDesign.createPrimitive(lib, "$_DFF_PN0_")
    c = naja.SNLScalarTerm.create(dff_pn0, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dff_pn0, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dff_pn0, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dff_pn0, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dff_pn0, c)


def constructDFF_PN1(lib):
    dff_pn1 = naja.SNLDesign.createPrimitive(lib, "$_DFF_PN1_")
    c = naja.SNLScalarTerm.create(dff_pn1, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dff_pn1, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dff_pn1, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dff_pn1, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dff_pn1, c)


def constructDFF_PP1(lib):
    dffe_pp1 = naja.SNLDesign.createPrimitive(lib, "$_DFF_PP1_")
    c = naja.SNLScalarTerm.create(dffe_pp1, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pp1, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pp1, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pp1, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pp1, c)


def constructDFFE_PP0P(lib):
    dffe_pp0p = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PP0P_")
    c = naja.SNLScalarTerm.create(dffe_pp0p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pp0p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pp0p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pp0p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pp0p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pp0p, c)


def constructDFFE_PP1P(lib):
    dffe_pp1p = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PP1P_")
    c = naja.SNLScalarTerm.create(dffe_pp1p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pp1p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pp1p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pp1p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pp1p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pp1p, c)


def constructDFFE_PP0N(lib):
    dffe_pp0n = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PP0N_")
    c = naja.SNLScalarTerm.create(dffe_pp0n, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pp0n, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pp0n, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pp0n, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pp0n, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pp0n, c)


def constructDFFE_PN0P(lib):
    dffe_pn0p = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PN0P_")
    c = naja.SNLScalarTerm.create(dffe_pn0p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pn0p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pn0p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pn0p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pn0p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pn0p, c)


def constructDFFE_PN0N(lib):
    dffe_pn0n = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PN0N_")
    c = naja.SNLScalarTerm.create(dffe_pn0n, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pn0n, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pn0n, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pn0n, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pn0n, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pn0n, c)


def constructDFFE_PN1P(lib):
    dffe_pn1p = naja.SNLDesign.createPrimitive(lib, "$_DFFE_PN1P_")
    c = naja.SNLScalarTerm.create(dffe_pn1p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(dffe_pn1p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(dffe_pn1p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(dffe_pn1p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(dffe_pn1p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(dffe_pn1p, c)


def constructSDFF_PP0(lib):
    sdff_pp0 = naja.SNLDesign.createPrimitive(lib, "$_SDFF_PP0_")
    c = naja.SNLScalarTerm.create(sdff_pp0, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdff_pp0, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdff_pp0, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdff_pp0, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdff_pp0, c)


def constructSDFFE_PP0N(lib):
    sdffe_pp0n = naja.SNLDesign.createPrimitive(lib, "$_SDFFE_PP0N_")
    c = naja.SNLScalarTerm.create(sdffe_pp0n, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffe_pp0n, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffe_pp0n, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffe_pp0n, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffe_pp0n, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffe_pp0n, c)


def constructSDFFE_PN0P(lib):
    sdffe_pn0p = naja.SNLDesign.createPrimitive(lib, "$_SDFFE_PN0P_")
    c = naja.SNLScalarTerm.create(sdffe_pn0p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffe_pn0p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffe_pn0p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffe_pn0p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffe_pn0p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffe_pn0p, c)


def constructSDFFE_PN0N(lib):
    sdffe_pn0n = naja.SNLDesign.createPrimitive(lib, "$_SDFFE_PN0N_")
    c = naja.SNLScalarTerm.create(sdffe_pn0n, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffe_pn0n, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffe_pn0n, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffe_pn0n, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffe_pn0n, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffe_pn0n, c)


def constructSDFFCE_PP0P(lib):
    sdffce_pp0p = naja.SNLDesign.createPrimitive(lib, "$_SDFFCE_PP0P_")
    c = naja.SNLScalarTerm.create(sdffce_pp0p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffce_pp0p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffce_pp0p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffce_pp0p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffce_pp0p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffce_pp0p, c)


def constructSDFFCE_PP1P(lib):
    sdffce_pp1p = naja.SNLDesign.createPrimitive(lib, "$_SDFFCE_PP1P_")
    c = naja.SNLScalarTerm.create(sdffce_pp1p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffce_pp1p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffce_pp1p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffce_pp1p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffce_pp1p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffce_pp1p, c)


def constructSDFFCE_PN0N(lib):
    sdffce_pn0n = naja.SNLDesign.createPrimitive(lib, "$_SDFFCE_PN0N_")
    c = naja.SNLScalarTerm.create(sdffce_pn0n, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffce_pn0n, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffce_pn0n, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffce_pn0n, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffce_pn0n, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffce_pn0n, c)


def constructSDFFCE_PN1P(lib):
    sdffce_pn1p = naja.SNLDesign.createPrimitive(lib, "$_SDFFCE_PN1P_")
    c = naja.SNLScalarTerm.create(sdffce_pn1p, naja.SNLTerm.Direction.Input, "C")
    naja.SNLScalarTerm.create(sdffce_pn1p, naja.SNLTerm.Direction.Input, "D")
    naja.SNLScalarTerm.create(sdffce_pn1p, naja.SNLTerm.Direction.Input, "E")
    naja.SNLScalarTerm.create(sdffce_pn1p, naja.SNLTerm.Direction.Output, "Q")
    naja.SNLScalarTerm.create(sdffce_pn1p, naja.SNLTerm.Direction.Input, "R")
    utils.constructSequentialPrimitive(sdffce_pn1p, c)


def load(db):
    logging.info("Loading Yosys primitives")
    lib = naja.NLLibrary.createPrimitives(db, "yosys")
    constructAND(lib)
    constructOR(lib)
    constructXOR(lib)
    constructXNOR(lib)
    constructMUX(lib)
    constructNOT(lib)
    constructDFFP(lib)
    constructDFFE_PP(lib)
    constructDFFE_PN(lib)
    constructDFF_PP0(lib)
    constructDFF_PN0(lib)
    constructDFF_PN1(lib)
    constructDFF_PP1(lib)
    constructDFFE_PP0P(lib)
    constructDFFE_PP1P(lib)
    constructDFFE_PP0N(lib)
    constructDFFE_PN0P(lib)
    constructDFFE_PN0N(lib)
    constructDFFE_PN1P(lib)
    constructSDFF_PP0(lib)
    constructSDFFE_PP0N(lib)
    constructSDFFE_PN0P(lib)
    constructSDFFE_PN0N(lib)
    constructSDFFCE_PP0P(lib)
    constructSDFFCE_PP1P(lib)
    constructSDFFCE_PN0N(lib)
    constructSDFFCE_PN1P(lib)
