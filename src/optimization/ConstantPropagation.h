// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"
#include <unordered_map>
#include "SNLTruthTable.h"

using namespace naja::SNL;
using namespace naja::DNL;

namespace naja::NAJA_OPT {

class ConstantPropagation {
 public:

  enum Type { AND = 1, OR, XOR, NAND, NOR, XNOR, INV, BUF, HA, DFF, MUX, OAI };

  ConstantPropagation() { dnl_ = get(); }
  void collectConstants();
  void run();
  // Getting all constant isos
  const std::set<DNLID>& getConstants0() const { return constants0_; }
  const std::set<DNLID>& getConstants1() const { return constants1_; }
  void initializeTypesID();
  const std::vector<std::tuple<std::vector<SNLID::DesignObjectID>, 
    std::vector<std::pair<SNLID::DesignObjectID, int>>, DNLID>>&
      getPartialConstantReaders() const {
    return partialConstantReaders_;
  }
  void setTruthTableEngine(bool value) {
    truthTableEngine_ = value;
  }

 private:
   
  unsigned computeOutputValueForConstantInstance(DNLID instanceID);
  unsigned computeOutputValueForPartiallyConstantInstance(DNLID instanceID);
  unsigned computeOutputValue(DNLID instanceID);
  //void computOuputValuesforHalfAdder(DNLID instanceID);
  void performConstantPropagationAnalysis();
  void propagateConstants();
  DNLFull* dnl_ = nullptr;
  std::unordered_map<SNLID::DesignID, DNLID> designObjectID2Type_;
  std::set<DNLID> initialConstants0_;
  std::set<DNLID> initialConstants1_;
  std::set<DNLID> constants0_;
  std::set<DNLID> constants1_;
  std::set<DNLID> partialConstantInstances_;
  std::vector<std::tuple<std::vector<SNLID::DesignObjectID>, SNLID::DesignObjectID, DNLID>>
      constant0Readers_;
  std::vector<SNLBitTerm*> constant0TopReaders_;
  std::vector<std::tuple<std::vector<SNLID::DesignObjectID>, SNLID::DesignObjectID, DNLID>>
      constant1Readers_;
  std::vector<std::tuple<std::vector<SNLID::DesignObjectID>, std::vector<std::pair<SNLID::DesignObjectID, int>>, DNLID>>
      partialConstantReaders_;
  std::vector<SNLBitTerm*> constant1TopReaders_;
  bool truthTableEngine_ = false;
};

}  // namespace naja::NAJA_OPT