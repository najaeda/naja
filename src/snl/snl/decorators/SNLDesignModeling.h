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

class SNLInstTerm;

/**
 * \brief SNLDesignModeling allows to add timing informations on primitives and blackboxes.
 */
class SNLDesignModeling {
  public:
    using TermArcs = std::set<SNLBitTerm*, SNLBitTerm::InDesignLess>;
    using Arcs = std::map<SNLBitTerm*, TermArcs, SNLBitTerm::InDesignLess>;
    struct TimingArcs {
      Arcs inputCombinatorialArcs_  {};
      Arcs outputCombinatorialArcs_ {};
      Arcs inputToClockArcs_        {};
      Arcs outputToClockArcs_       {};
    };
    using Parameter = std::pair<std::string, std::string>;
    using ParameterizedArcs = std::map<Parameter, TimingArcs>;
    enum Type { NO_PARAMETER, PARAMETERIZED };
    using TimingModel = std::variant<TimingArcs, ParameterizedArcs>;
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

    SNLDesignModeling(Type type): type_(type) {}
    Type getType() const { return type_; }
  private:
    void addCombinatorialArcs_(SNLBitTerm* input, SNLBitTerm* output);
    void addCombinatorialArcs_(const Parameter& parameter, SNLBitTerm* input, SNLBitTerm* output);
    const TimingArcs* getTimingArcs() const;
    NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLBitTerm* term) const;
    NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLBitTerm* term) const;
    NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLInstTerm* term) const;
    NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLInstTerm* term) const;
    bool isClock_(const SNLBitTerm* term) const;
    Type        type_             { NO_PARAMETER };
    Parameter   defaultParameter_ { std::make_pair(std::string(), std::string()) };
    TimingModel model_            {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_