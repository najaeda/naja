import logging
from najaeda import naja

def constructAND2(lib):
    and2 = naja.SNLDesign.createPrimitive(lib, "AND2")
    i0 = naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(and2, naja.SNLTerm.Direction.Output, "O")

def constructOR2(lib):
    or2 = naja.SNLDesign.createPrimitive(lib, "OR2")
    i0 = naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "I0")
    i1 = naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Input, "I1")
    o = naja.SNLScalarTerm.create(or2, naja.SNLTerm.Direction.Output, "O")

def load(db):
    logging.info("Loading Custom primitives")
    lib = naja.NLLibrary.createPrimitives(db, "custom_lib")
    constructAND2(lib)
    constructOR2(lib)