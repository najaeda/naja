# SPDX-FileCopyrightText: 2024 The Naja authors
# SPDX-License-Identifier: Apache-2.0

"""
Serialization helpers for exposing NajaEDA objects over the WebSocket API.
These functions convert internal objects into JSON-serializable dicts.
"""

#from __future__ import annotations

#from typing import Optional, Dict, Any

from najaeda import naja


def direction_to_int(direction: naja.SNLTerm.Direction) -> int:
    """
    Convert an SNLTerm.Direction enum to a compact integer representation:
      0 -> Input
      1 -> Output
      2 -> InOut / others
    """
    if direction == naja.SNLTerm.Direction.Input:
        return 0
    elif direction == naja.SNLTerm.Direction.Output:
        return 1
    else:
        return 2


def serialize_design_ref(model: naja.SNLDesign):
    """
    Serialize a design reference into db / library / design IDs.
    """
    return {
        "db_id": model.getDB().getID(),
        "library_id": model.getLibrary().getID(),
        "design_id": model.getID(),
    }


def serialize_model(model: naja.SNLDesign, child_id: int, name: str):
    """
    Serialize a model (SNLDesign) plus its position as a child instance (child_id, name).
    """
    return {
        "name": name,
        "child_id": child_id,
        "model_name": model.getName(),
        "design_ref": serialize_design_ref(model),
        "has_terms": model.hasTerms(),
        "has_primitives": model.hasPrimitiveInstances(),
        "has_instances": model.hasNonPrimitiveInstances(),
    }


def serialize_term(term: naja.SNLTerm):
    """
    Serialize a top-level term (design term).
    """
    msb = None
    lsb = None
    bit = None

    if isinstance(term, naja.SNLBusTerm):
        msb = term.getMSB()
        lsb = term.getLSB()

    if isinstance(term, naja.SNLBusTermBit):
        bit = term.getBit()

    return {
        "name": term.getName(),
        "child_id": term.getID(),
        "direction": direction_to_int(term.getDirection()),
        "msb": msb,
        "lsb": lsb,
        "bit": bit,
    }


def serialize_equipotential_occurrence(occ: naja.SNLOccurrence):
    """
    Serialize a SNLOccurrence for an inst-term that belongs to an equipotential.
    """
    path = [[inst.getName(), inst.getID()] for inst in occ.getPath().getInstances()]
    inst_term = occ.getInstTerm()
    inst = inst_term.getInstance()

    # Last element is the instance hosting the term
    path.append([inst.getName(), inst.getID()])

    term = inst_term.getBitTerm()
    bit = term.getBit() if isinstance(term, naja.SNLBusTermBit) else None

    return {
        "path": path,
        "term_id": term.getID(),
        "name": term.getName(),
        "direction": direction_to_int(term.getDirection()),
        "bit": bit,
    }


def serialize_equipotential_term(term: naja.SNLTerm):
    """
    Serialize a top-level term belonging to an equipotential.
    """
    bit = term.getBit() if isinstance(term, naja.SNLBusTermBit) else None
    return {
        "name": term.getName(),
        "child_id": term.getID(),
        "direction": direction_to_int(term.getDirection()),
        "bit": bit,
    }