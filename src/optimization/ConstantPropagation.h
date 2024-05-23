// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "DNL.h"
#include <unordered_map>

using namespace naja::SNL;
using namespace naja::DNL;

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
  const std::vector<std::tuple<std::vector<SNLInstance*>, std::vector<std::pair<SNLInstTerm*, int>>, DNLID>>&
      getPartialConstantReaders() const {
    return partialConstantReaders_;
  }

 private:
   
  unsigned computeOutputValueForConstantInstance(DNLID instanceID);
  unsigned computeOutputValueForPartiallyConstantInstance(DNLID instanceID);
  //void computOuputValuesforHalfAdder(DNLID instanceID);
  void performConstantPropagationAnalysis();
  void propagateConstants();
  void changeDriverToLocal0(SNLInstTerm* term, DNLID id);
  void changeDriverToLocal1(SNLInstTerm* term, DNLID id);
  DNLFull* dnl_ = nullptr;
  std::unordered_map<SNLID::DesignID, DNLID> designObjectID2Type_;
  std::set<DNLID> constants0_;
  std::set<DNLID> constants1_;
  std::set<DNLID> partialConstantInstances_;
  std::vector<std::tuple<std::vector<SNLInstance*>, SNLInstTerm*, DNLID>>
      constant0Readers_;
  std::vector<SNLBitTerm*> constant0TopReaders_;
  std::vector<std::tuple<std::vector<SNLInstance*>, SNLInstTerm*, DNLID>>
      constant1Readers_;
  std::vector<std::tuple<std::vector<SNLInstance*>, std::vector<std::pair<SNLInstTerm*, int>>, DNLID>>
      partialConstantReaders_;
  std::vector<SNLBitTerm*> constant1TopReaders_;
};
