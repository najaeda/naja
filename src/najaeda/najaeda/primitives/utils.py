# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

try:
    from najaeda import naja
except ImportError:  # pragma: no cover
    import naja


def _bit_terms(terms):
    if terms is None:
        return []
    if isinstance(terms, (list, tuple, set)):
        result = []
        for term in terms:
            result.extend(_bit_terms(term))
        return result
    if hasattr(terms, "getBits"):
        return list(terms.getBits())
    return [terms]


def setTermRole(terms, role, active_level=None):
    """Assign a timing-model role to one or more primitive bit terms."""
    if active_level is None:
        active_level = naja.SNLActiveLevel.NA
    for term in _bit_terms(terms):
        term.setRole(role, active_level)


def constructSequentialPrimitive(design, clk, term_roles=None):
    """Add sequential timing arcs and term roles to a primitive.

    Inputs other than ``clk`` default to ``DataInput`` and outputs default to
    ``DataOutput``. ``term_roles`` can override those defaults with either a
    role or a ``(role, active_level)`` pair.
    """
    input_terms = []
    output_terms = []
    for term in design.getBitTerms():
        if term == clk:
            continue
        if term.getDirection() == naja.SNLTerm.Direction.Input:
            input_terms.append(term)
        elif term.getDirection() == naja.SNLTerm.Direction.Output:
            output_terms.append(term)
    naja.SNLDesign.addClockToOutputsArcs(clk, output_terms)
    naja.SNLDesign.addInputsToClockArcs(input_terms, clk)
    setTermRole(clk, naja.SNLTermRole.Clock)
    setTermRole(input_terms, naja.SNLTermRole.DataInput)
    setTermRole(output_terms, naja.SNLTermRole.DataOutput)
    for terms, role_specification in (term_roles or {}).items():
        if isinstance(role_specification, tuple):
            role, active_level = role_specification
            setTermRole(terms, role, active_level)
        else:
            setTermRole(terms, role_specification)
