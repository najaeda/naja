# SPDX-License-Identifier: Apache-2.0

"""
WebSocket protocol layer for NajaEDA remote access.

This module knows how to:
- interpret incoming JSON requests
- talk to NLUniverse / SNL objects
- return JSON-serializable responses via the websocket
"""

#from __future__ import annotations

import json
import logging
#from typing import Any, Dict, Optional, List, Tuple

#from najaeda import naja
#from najaeda.remote.serializer import (
#    serialize_model,
#    serialize_term,
#    serialize_equipotential_occurrence,
#    serialize_equipotential_term,
#)

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def get_design_ref(ref_msg: Optional[Dict[str, Any]]) -> Optional[Tuple[int, int, int]]:
    if not ref_msg:
        return None
    return (
        ref_msg.get("db_id"),
        ref_msg.get("library_id"),
        ref_msg.get("design_id"),
    )


def get_path_from_ids(top: naja.SNLDesign, instance_ids: List[int]) -> naja.SNLPath:
    current = top
    path = naja.SNLPath()
    for inst_id in instance_ids:
        instance = current.getInstanceByID(inst_id)
        if not instance:
            raise ValueError(f"Instance with ID {inst_id} not found in path resolution")
        path = naja.SNLPath(path, instance)
        current = instance.getModel()
    return path


async def send_error(
    websocket,
    response_type: str,
    gui_id: int = 0,
    message: Optional[str] = None,
):
    payload: Dict[str, Any] = {
        "response": response_type,
        "gui_id": gui_id,
        "error": True,
    }
    if message is not None:
        payload["message"] = message
    await websocket.send(json.dumps(payload))


# ---------------------------------------------------------------------------
# Request handlers
# ---------------------------------------------------------------------------

async def handle_load_root(websocket, gui_id: int):
    u = naja.NLUniverse.get()
    top = u.getTopDesign()
    if not top:
        await send_error(websocket, "root_response", gui_id, "Top design not set")
        return

    await websocket.send(
        json.dumps(
            {
                "response": "root_response",
                "gui_id": gui_id,
                "root": serialize_model(top, 0, top.getName()),
            }
        )
    )


async def handle_load_instance_like(
    websocket,
    req_type: str,
    design_ref_msg: Dict[str, Any],
    gui_id: int,
):
    ref = get_design_ref(design_ref_msg)
    if ref is None:
        await send_error(websocket, f"{req_type}_response", gui_id, "Missing design_ref")
        return

    u = naja.NLUniverse.get()
    design = u.getSNLDesign(ref)
    logger.debug("Handling %s for design_ref=%s -> %s", req_type, ref, design)

    if not design:
        await send_error(websocket, f"{req_type}_response", gui_id, "Design not found")
        return

    if req_type == "load_instance":
        await websocket.send(
            json.dumps(
                {
                    "response": "instance_response",
                    "gui_id": gui_id,
                    "instance": {
                        "design_ref": {
                            "db_id": design.getDB().getID(),
                            "library_id": design.getLibrary().getID(),
                            "design_id": design.getID(),
                        },
                        "has_terms": design.hasTerms(),
                        "has_primitives": design.hasPrimitiveInstances(),
                        "has_instances": design.hasNonPrimitiveInstances(),
                    },
                }
            )
        )
        return

    if req_type in {"load_primitives", "load_instances"}:
        children = []
        instances = (
            design.getPrimitiveInstances()
            if req_type == "load_primitives"
            else design.getNonPrimitiveInstances()
        )

        for instance in instances:
            model = instance.getModel()
            children.append(serialize_model(model, instance.getID(), instance.getName()))

        response_type = req_type.replace("load_", "") + "_response"
        await websocket.send(
            json.dumps(
                {
                    "response": response_type,
                    "gui_id": gui_id,
                    "children": children,
                }
            )
        )
        return

    if req_type == "load_terms":
        terms_payload = [serialize_term(term) for term in design.getTerms()]
        await websocket.send(
            json.dumps(
                {
                    "response": "terms_response",
                    "gui_id": gui_id,
                    "children": terms_payload,
                }
            )
        )
        return


async def handle_load_equipotential(websocket, request: Dict[str, Any], gui_id: int):
    path_ids: List[int] = request.get("path", []) or []
    term_id = request.get("term_id")
    bit = request.get("bit", None)

    if term_id is None:
        await send_error(websocket, "equipotential_response", gui_id, "Missing term_id")
        return

    logger.debug(
        "LoadEquipotential path=%s term_id=%s bit=%s", path_ids, term_id, bit
    )

    u = naja.NLUniverse.get()
    top = u.getTopDesign()
    if top is None:
        await send_error(
            websocket, "equipotential_response", gui_id, "Top design not set"
        )
        return

    path = get_path_from_ids(top, path_ids) if path_ids else naja.SNLPath()
    logger.debug("Resolved path: %s", path)

    if path.empty():
        design = top
        term = design.getTermByID(term_id)
        if term is None:
            await send_error(
                websocket, "equipotential_response", gui_id, "Term not found"
            )
            return

        if bit is not None:
            if not isinstance(term, naja.SNLBusTerm):
                await send_error(
                    websocket,
                    "equipotential_response",
                    gui_id,
                    "Bit index provided but term is not a bus",
                )
                return
            start_point = term.getBusTermBit(bit)
        else:
            start_point = term
    else:
        design = path.getModel()
        term = design.getTermByID(term_id)
        if term is None:
            await send_error(
                websocket, "equipotential_response", gui_id, "Term not found"
            )
            return

        instance = path.getTailInstance()
        if bit is not None:
            if not isinstance(term, naja.SNLBusTerm):
                await send_error(
                    websocket,
                    "equipotential_response",
                    gui_id,
                    "Bit index provided but term is not a bus",
                )
                return
            term = term.getBusTermBit(bit)

        inst_term = instance.getInstTerm(term)
        head_path = path.getHeadPath()
        start_point = naja.SNLOccurrence(head_path, inst_term)

    logger.debug("Start point for equipotential: %s", start_point)

    equipotential = naja.SNLEquipotential(start_point)

    occurrences = [
        serialize_equipotential_occurrence(occ)
        for occ in equipotential.getInstTermOccurrences()
    ]
    terms = [serialize_equipotential_term(t) for t in equipotential.getTerms()]

    await websocket.send(
        json.dumps(
            {
                "response": "equipotential_response",
                "gui_id": gui_id,
                "occurrences": occurrences,
                "terms": terms,
            }
        )
    )


# ---------------------------------------------------------------------------
# Main entry point for the connection handler
# ---------------------------------------------------------------------------

async def handle_request(websocket, request: Dict[str, Any]):
    """
    Entry point used by the connection-level handler in server.py.

    It:
    - inspects the "request" field
    - dispatches to the appropriate handler
    - ensures consistent error responses
    """
    req_type = request.get("request")
    design_ref_message = request.get("design_ref")
    gui_id = request.get("gui_id", 0)

    logger.debug("Received request type=%s gui_id=%s", req_type, gui_id)

    if req_type is None:
        await send_error(websocket, "unknown_response", gui_id, "Missing 'request' field")
        return

    if req_type == "load_root":
        await handle_load_root(websocket, gui_id)
        return

    if req_type in {"load_instance", "load_primitives", "load_instances", "load_terms"}:
        if not design_ref_message:
            await send_error(
                websocket,
                f"{req_type}_response",
                gui_id,
                "Missing 'design_ref' for this request type",
            )
            return
        await handle_load_instance_like(websocket, req_type, design_ref_message, gui_id)
        return

    if req_type == "load_equipotential":
        await handle_load_equipotential(websocket, request, gui_id)
        return

    # Unknown request type
    logger.warning("Unknown request type: %s", req_type)
    await send_error(websocket, "unknown_response", gui_id, f"Unknown request '{req_type}'")