# SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

from najaeda import naja

def constructSequentialPrimitive(design, clk):
    input_terms = []
    output_terms = []
    for term in design.getBitTerms():
        if term == clk:
            pass
        if term.getDirection() == naja.SNLTerm.Direction.Input:
            input_terms.append(term)
        elif term.getDirection() == naja.SNLTerm.Direction.Output:
            output_terms.append(term)
    naja.SNLDesign.addClockToOutputsArcs(clk, output_terms)
    naja.SNLDesign.addInputsToClockArcs(input_terms, clk)