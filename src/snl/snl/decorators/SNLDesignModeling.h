// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_MODELING_H_
#define __SNL_DESIGN_MODELING_H_

#include <set>
#include <list>
#include "SNLBitTerm.h"

namespace naja { namespace SNL {

/**
 * \brief SNLDesignModeling allows to add timing informations on primitives and blackboxes.
 */
class SNLDesignModeling {
  public:
    using TermArcs = std::set<SNLBitTerm*, SNLBitTerm::InDesignLess>;
    using Arcs = std::map<SNLBitTerm*, TermArcs, SNLBitTerm::InDesignLess>;
    using BitTerms = std::list<SNLBitTerm*>;

    static void addCombinatorialArcs(const BitTerms& inputs, const BitTerms& outputs);
    static void addInputsToClockArcs(const BitTerms& inputs, SNLBitTerm* clock);
    static void addClockToOutputsArcs(SNLBitTerm* clock, const BitTerms& outputs);
    static NajaCollection<SNLBitTerm*> getCombinatorialOutputs(SNLBitTerm* term);
    static NajaCollection<SNLBitTerm*> getCombinatorialInputs(SNLBitTerm* term);
    static NajaCollection<SNLBitTerm*> getOutputRelatedClocks(SNLBitTerm* term);
    static NajaCollection<SNLBitTerm*> getInputRelatedClocks(SNLBitTerm* term);
    static bool getClockRelatedOutputs(const SNLBitTerm* term);
    static bool getClockRelatedInputs(const SNLBitTerm* term);
    private:
      void addCombinatorialArcs_(SNLBitTerm* input, SNLBitTerm* output);
      NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLBitTerm* term) const;
      NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLBitTerm* term) const;
      bool isClock_(const SNLBitTerm* term) const;
      Arcs inputCombinatorialArcs_  {};
      Arcs outputCombinatorialArcs_ {};
      Arcs inputToClockArcs_        {};
      Arcs outputToClockArcs_       {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_