// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <set>
#include <list>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "SNLBitTerm.h"
#include "SNLTruthTable.h"

namespace naja::NL {

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
    // Generic black-box memory description used by frontends and downstream
    // tools. The terms are intentionally kept as SNL bit terms instead of
    // frontend-specific names so users like SEC can model memories without
    // hardcoding fakeram/CVA6 pin naming conventions.
    enum class MemoryResetMode {
      None,
      AsyncLow,
      AsyncHigh,
      SyncLow,
      SyncHigh
    };
    struct MemoryReadPort {
      BitTerms address {};
      BitTerms data    {};
      BitTerms enables {};
    };
    struct MemoryWritePort {
      BitTerms address {};
      BitTerms data    {};
      BitTerms mask    {};
      BitTerms enables {};
      // Extra write-side dependency buses are preserved for completeness, but
      // callers may reject them if they only support address/data/mask/enable
      // memories.
      std::vector<BitTerms> extraWriteInputs {};
    };
    struct MemoryInterface {
      size_t width  {0};
      size_t depth  {0};
      size_t abits  {0};
      MemoryResetMode resetMode {MemoryResetMode::None};
      SNLBitTerm* clock {nullptr};
      SNLBitTerm* reset {nullptr};
      std::vector<MemoryReadPort> readPorts   {};
      std::vector<MemoryWritePort> writePorts {};

      bool isValid() const {
        return clock != nullptr && width > 0 && depth > 0 && abits > 0 &&
               !readPorts.empty() && !writePorts.empty();
      }
    };

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
    // Attach or query a memory interface on a primitive design. For DB0 memory
    // primitives the interface is synthesized from the existing DB0 signature,
    // and for instances the returned interface is filtered to connected ports.
    static void setMemoryInterface(SNLDesign* design, const MemoryInterface& interface);
    static bool hasMemoryInterface(const SNLDesign* design);
    static MemoryInterface getMemoryInterface(const SNLDesign* design);
    static MemoryInterface getMemoryInterface(const SNLInstance* instance);

    static void setTruthTable(SNLDesign* design, const SNLTruthTable& truthTable);
    static void setTruthTables(SNLDesign* design, const std::vector<SNLTruthTable>& truthTable);
    static SNLTruthTable getTruthTable(const SNLDesign* design);
    static SNLTruthTable getTruthTable(const SNLDesign* design, size_t flatTermID);
    static bool hasModeling(const SNLDesign* design);
    static bool isSequential(const SNLDesign* design);
    static bool isConst0(const SNLDesign* design);
    static bool isConst1(const SNLDesign* design);
    static bool isConst(const SNLDesign* design);
    static bool isInv(const SNLDesign* design);
    static bool isBuf(const SNLDesign* design);
    static size_t getTruthTableCount(const SNLDesign* design);
    static bool areDependenciesDefined(const SNLBitTerm* term);
    SNLDesignModeling(Type type);
    Type getType() const { return type_; }
  private:
    
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output);
    void addCombinatorialArc_(SNLBitTerm* input, SNLBitTerm* output, const std::string& parameterValue);
    void addInputToClockArc_(SNLBitTerm* input, SNLBitTerm* clock);
    void addClockToOutputArc_(SNLBitTerm* clock, SNLBitTerm* output);
    const TimingArcs* getTimingArcs(const SNLInstance* instance=nullptr) const;
    TimingArcs* getOrCreateTimingArcs(const std::string& parameterValue=std::string());
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
    void setMemoryInterface_(const MemoryInterface& interface) { memoryInterface_ = interface; }
    bool hasMemoryInterface_() const { return memoryInterface_.has_value(); }
    MemoryInterface getMemoryInterface_() const { return *memoryInterface_; }
    Type          type_       { NO_PARAMETER };
    Parameter     parameter_  {};
    TimingModel   model_      {};
    std::optional<MemoryInterface> memoryInterface_ {};
};

}  // namespace naja::NL
