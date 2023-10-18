// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_MODELING_H_
#define __SNL_DESIGN_MODELING_H_

#include <set>
#include "SNLBitTerm.h"

namespace naja { namespace SNL {

/**
 * \brief SNLDesignModeling allows to add timing informations on primitives and blackboxes.
 */
class SNLDesignModeling {
  public:
    using TermDependencies = std::set<const SNLBitTerm*> ; //, SNLBitTerm::InDesignLess>;
    using Dependencies = std::map<const SNLBitTerm*, TermDependencies, SNLBitTerm::InDesignLess>;

    static void addCombinatorialDependency(const SNLBitTerm* input, const SNLBitTerm* output);
    //static TimingArc* getTimingArc(const SNLTerm* term) { return nullptr; }
    private:
      void addCombinatorialDependency_(const SNLBitTerm* input, const SNLBitTerm* output);
      Dependencies inputCombinatorialDependencies_ {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_