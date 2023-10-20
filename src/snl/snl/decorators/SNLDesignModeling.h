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
    using TermDependencies = std::set<SNLBitTerm*, SNLBitTerm::InDesignLess>;
    using Dependencies = std::map<SNLBitTerm*, TermDependencies, SNLBitTerm::InDesignLess>;
    using BitTerms = std::list<SNLBitTerm*>;

    static void addCombinatorialDependency(const BitTerms& inputs, const BitTerms& outputs);
    static NajaCollection<SNLBitTerm*> getCombinatorialOutputs(SNLBitTerm* term);
    static NajaCollection<SNLBitTerm*> getCombinatorialInputs(SNLBitTerm* term);

    private:
      void addCombinatorialDependency_(SNLBitTerm* input, SNLBitTerm* output);
      NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLBitTerm* term) const;
      NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLBitTerm* term) const;
      Dependencies inputCombinatorialDependencies_ {};
      Dependencies outputCombinatorialDependencies_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_