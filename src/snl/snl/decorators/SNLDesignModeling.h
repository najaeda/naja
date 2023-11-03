// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_MODELING_H_
#define __SNL_DESIGN_MODELING_H_

#include <set>
#include <list>
#include <variant>
#include "SNLBitTerm.h"

namespace naja { namespace SNL {

class SNLInstance;
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
      Arcs clockToInputArcs_        {};
      Arcs outputToClockArcs_       {};
      Arcs clockToOutputArcs_       {};
    };
    //First is parameter name, second is default value
    using Parameter = std::pair<std::string, std::string>;
    using ParameterizedArcs = std::map<std::string, TimingArcs>;
    enum Type { PARAMETERIZED, NO_PARAMETER };
    using TimingModel = std::variant<ParameterizedArcs, TimingArcs>;
    using BitTerms = std::list<SNLBitTerm*>;

    //In case the Timing Modeling of a design depends on a parameter
    //Following method must be called prior to others
    static void setParameter(SNLDesign* design, const std::string& name, const std::string& defaultValue);
    static void addCombinatorialArcs(const BitTerms& inputs, const BitTerms& outputs);
    static void addCombinatorialArcs(const std::string& parameterValue, const BitTerms& inputs, const BitTerms& outputs);
    static void addInputsToClockArcs(const BitTerms& inputs, SNLBitTerm* clock);
    static void addClockToOutputsArcs(SNLBitTerm* clock, const BitTerms& outputs);
    static NajaCollection<SNLBitTerm*> getCombinatorialOutputs(SNLBitTerm* input);
    static NajaCollection<SNLBitTerm*> getCombinatorialInputs(SNLBitTerm* output);
    static NajaCollection<SNLInstTerm*> getCombinatorialOutputs(SNLInstTerm* iinput);
    static NajaCollection<SNLInstTerm*> getCombinatorialInputs(SNLInstTerm* ioutput);
    static NajaCollection<SNLBitTerm*> getOutputRelatedClocks(SNLBitTerm* output);
    static NajaCollection<SNLBitTerm*> getInputRelatedClocks(SNLBitTerm* input);
    static NajaCollection<SNLBitTerm*> getClockRelatedOutputs(SNLBitTerm* clock);
    static NajaCollection<SNLBitTerm*> getClockRelatedInputs(SNLBitTerm* clock);
    static NajaCollection<SNLInstTerm*> getOutputRelatedClocks(SNLInstTerm* ioutput);
    static NajaCollection<SNLInstTerm*> getInputRelatedClocks(SNLInstTerm* iinput);
    static NajaCollection<SNLInstTerm*> getClockRelatedOutputs(SNLInstTerm* iclock);
    static NajaCollection<SNLInstTerm*> getClockRelatedInputs(SNLInstTerm* iclock);

    SNLDesignModeling(Type type);
    Type getType() const { return type_; }
  private:
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output);
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output, const std::string& parameterValue);
    void addInputToClockArc_(SNLBitTerm* input, SNLBitTerm* clock);
    void addClockToOutputArc_(SNLBitTerm* clock, SNLBitTerm* output);
    const TimingArcs* getTimingArcs(const SNLInstance* instance=nullptr) const;
    TimingArcs* getOrCreateTimingArcs(const std::string& parameterValue=std::string());
    //bool isClock_(const SNLBitTerm* term) const;
    NajaCollection<SNLBitTerm*> getCombinatorialOutputs_(SNLBitTerm* input) const;
    NajaCollection<SNLBitTerm*> getCombinatorialInputs_(SNLBitTerm* output) const;
    NajaCollection<SNLInstTerm*> getCombinatorialOutputs_(SNLInstTerm* iinput) const;
    NajaCollection<SNLInstTerm*> getCombinatorialInputs_(SNLInstTerm* ioutput) const;
    NajaCollection<SNLBitTerm*> getClockRelatedOutputs_(SNLBitTerm* clock) const;
    NajaCollection<SNLBitTerm*> getClockRelatedInputs_(SNLBitTerm* clock) const;
    NajaCollection<SNLBitTerm*> getOutputRelatedClocks_(SNLBitTerm* output) const;
    NajaCollection<SNLBitTerm*> getInputRelatedClocks_(SNLBitTerm* input) const;
    NajaCollection<SNLInstTerm*> getClockRelatedOutputs_(SNLInstTerm* iclock) const;
    NajaCollection<SNLInstTerm*> getClockRelatedInputs_(SNLInstTerm* iclock) const;
    NajaCollection<SNLInstTerm*> getOutputRelatedClocks_(SNLInstTerm* ioutput) const;
    NajaCollection<SNLInstTerm*> getInputRelatedClocks_(SNLInstTerm* iinput) const;
    Type        type_             { NO_PARAMETER };
    Parameter   parameter_        {};
    TimingModel model_            {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_MODELING_H_
