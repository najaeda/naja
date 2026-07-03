#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Build and validate deterministic logic-cone signatures for external RTL."""

from __future__ import annotations

from collections import Counter
from typing import Any


SIGNATURE_FIELDS = (
    "nodes",
    "edges",
    "leaves",
    "roots",
    "registers",
    "ports",
    "blackboxes",
    "internal",
)


def summarize_cone(cone: Any) -> dict[str, int]:
    """Return stable structural counts from a ``naja.LogicCone``."""
    nodes = cone.get_nodes()
    kinds = Counter(node[2] for node in nodes)
    return {
        "nodes": len(nodes),
        "edges": sum(len(node[3]) for node in nodes),
        "leaves": len(cone.get_leaves()),
        "roots": kinds["root"],
        "registers": kinds["flop"],
        "ports": kinds["ports"],
        "blackboxes": kinds["blackbox"],
        "internal": kinds["internal"],
    }


def probe_name(probe: dict[str, Any]) -> str:
    name = str(probe["term"])
    if "bit" in probe:
        name += f"[{probe['bit']}]"
    return name


def build_signatures(
    top: Any, probes: list[dict[str, Any]], naja: Any
) -> list[dict[str, Any]]:
    """Build configured top-level term cones and return their signatures."""
    signatures = []
    for probe in probes:
        term_name = probe.get("term")
        if not isinstance(term_name, str) or not term_name:
            raise RuntimeError(f"logic-cone probe has invalid term: {term_name!r}")
        term = top.get_term(term_name)
        if term is None:
            raise RuntimeError(f"logic-cone probe term was not found: {term_name}")
        snl_term = term.get_snl_term()
        if "bit" in probe:
            snl_term = snl_term.getBusTermBit(int(probe["bit"]))
            if snl_term is None:
                raise RuntimeError(
                    f"logic-cone probe bit was not found: {probe_name(probe)}"
                )

        direction = probe.get("direction", "fanin")
        cone = naja.LogicCone(naja.SNLOccurrence(snl_term), direction)
        signature = {
            "root": probe_name(probe),
            "direction": direction,
            **summarize_cone(cone),
        }
        signatures.append(signature)
    return signatures


def validate_signatures(
    probes: list[dict[str, Any]], signatures: list[dict[str, Any]]
) -> None:
    """Require every configured expected count to match its actual value."""
    if len(probes) != len(signatures):
        raise RuntimeError(
            f"logic-cone signature count mismatch: expected {len(probes)}, "
            f"got {len(signatures)}"
        )
    failures = []
    for probe, signature in zip(probes, signatures):
        expected = probe.get("expected")
        if not isinstance(expected, dict) or not expected:
            raise RuntimeError(
                f"logic-cone probe has no expected signature: {probe_name(probe)}"
            )
        unknown = sorted(set(expected).difference(SIGNATURE_FIELDS))
        if unknown:
            raise RuntimeError(
                f"logic-cone probe {probe_name(probe)} has unknown signature "
                f"fields: {unknown}"
            )
        for field, expected_value in expected.items():
            actual_value = signature[field]
            if actual_value != expected_value:
                failures.append(
                    f"{probe_name(probe)} {field}: expected {expected_value}, "
                    f"got {actual_value}"
                )
    if failures:
        raise RuntimeError("logic-cone signature mismatch:\n  " + "\n  ".join(failures))
