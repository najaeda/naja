// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_MODELING_DECORATOR_H_
#define __SNL_DESIGN_MODELING_DECORATOR_H_

namespace naja { namespace SNL {

class SNLDesign;
class SNLTerm;

/**
 * \brief SNLDesignModeling allows to add timing informations on primitives and blackboxes.
 */
class SNLDesignModeling {
  public:
    struct TimingArc {
      SNLTerm* input;
      SNLTerm* output;
    };
    static void addTimingArc(const SNLTerm* input, const SNLTerm* output);
    static TimingArc* getTimingArc(const SNLTerm* term) { return nullptr; }
    
  //private:
};

class SNLDesignModelingDecorator {
  public:
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_DECORATOR_H_