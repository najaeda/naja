// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <spdlog/spdlog.h>
#include "Reduction.h"
#include <ranges>
#include "SNLDesignTruthTable.h"
#include "SNLTruthTable.h"
#include "Utils.h"

using namespace naja::DNL;
using namespace naja::SNL;
using namespace naja::NAJA_OPT;

//#define DEBUG_PRINTS

ReductionOptimization::ReductionOptimization(
    const std::vector<std::tuple<std::vector<SNLID::DesignObjectID>,
                                 std::vector<std::pair<SNLID::DesignObjectID, int>>,
                                 DNLID>>& partialConstantReaders)
    : partialConstantReaders_(partialConstantReaders) {}

void ReductionOptimization::run() {
  for (auto& partialConstantReader : partialConstantReaders_) {
    reducPartialConstantInstance(partialConstantReader);
  }
  report_ = collectStatistics();
  //spdlog::info(report_);
  destroy();
}

SNLTruthTable ReductionOptimization::reduceTruthTable(SNLInstance* uniquifiedCandidate,
    const SNLTruthTable& truthTable,
    const std::vector<std::pair<SNLID::DesignObjectID, int>>& constTerms) {
  assert(constTerms.size() <= truthTable.size());
  std::map<size_t, size_t> termID2index;
  size_t index = 0;
  using ConstantInput = std::pair<uint32_t, bool>;
  using ConstantInputs = std::vector<ConstantInput>;
  ConstantInputs constInputs;
  for (auto term : uniquifiedCandidate->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      continue;
    }
    termID2index[term->getBitTerm()->getID()] = index;
    index++;
  }
  for (auto& constTerm : constTerms) {
    constInputs.push_back(
        ConstantInput(termID2index[constTerm.first],
                      constTerm.second));
  }
  return truthTable.getReducedWithConstants(constInputs);
}

void ReductionOptimization::replaceInstance(
    SNLInstance* instance,
    const std::pair<SNLDesign*, SNLLibraryTruthTables::Indexes>& result) {
  reductionStatistics_[std::pair<std::string, std::string>(instance->getModel()->getName().getString(), 
    result.first->getName().getString())]++;
  SNLDesign* design = instance->getDesign();
  SNLDesign* reducedDesign = result.first;
  SNLInstance* reducedInstance = SNLInstance::create(
      design, reducedDesign,
      SNLName(std::string(instance->getName().getString()) + "_reduced"));
  std::vector<SNLInstTerm*> reducedInstTerms;
  SNLInstTerm* output = nullptr;
  SNLInstTerm* reducedOutput = nullptr;
  for (auto term : reducedInstance->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      reducedOutput = term;
      continue;
    }
    reducedInstTerms.push_back(term);
  }
  size_t index = 0;
  size_t originNonConstantIndex = 0;
  for (auto term : instance->getInstTerms()) {
    if (term->getDirection() != SNLInstTerm::Direction::Input) {
      output = term;
      break;
    }
  }
  for (auto term : instance->getInstTerms()) {
    SNLBitNet* bitNet = term->getNet();
    term->setNet(nullptr);
    if (bitNet->isConstant() || reducedInstTerms.empty()) {
      continue;
    }
    originNonConstantIndex++;
    if (std::find(result.second.begin(), result.second.end(),
                  originNonConstantIndex) != result.second.end()) {
      continue;
    }
    reducedInstTerms[index]->setNet(bitNet);
    index++;
    if (index == reducedInstTerms.size()) {
      break;
    }
  }
  reducedOutput->setNet(output->getNet());
  output->setNet(nullptr);
  instance->destroy();
}

void ReductionOptimization::reducPartialConstantInstance(
    std::tuple<std::vector<SNLID::DesignObjectID>,
               std::vector<std::pair<SNLID::DesignObjectID, int>>,
               DNLID>& candidate) {
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("reducPartialConstantInstance Reducing partial constant instance\n");
  // LCOV_EXCL_STOP
#endif
  auto library = *(SNLUniverse::get()->getTopDesign()->getDB()->getPrimitiveLibraries().begin());
  Uniquifier uniquifier(std::get<0>(candidate), std::get<2>(candidate));
  uniquifier.process();
  SNLInstance* uniquifiedCandidate = uniquifier.getPathUniq().back();
  /*if (!uniquifiedCandidate) {uniquifier.getPathUniq().back()
    std::ostringstream reason;
    auto instance = std::get<0>(candidate).back();
    reason << "Uniquified candidate is null for instance: "
      << instance->getName().getString() << " in design: "
      << instance->getDesign()->getName().getString();
    throw SNLException(reason.str());
  }*/
  SNLTruthTable invTruthTable =
      SNLDesignTruthTable::getTruthTable(uniquifiedCandidate->getModel());
  if (!invTruthTable.isInitialized()) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "reducPartialConstantInstance Truth table is not initialized for "
        "design: %s\n",
        unqiuifedCandidate->getModel()->getName().getString().c_str());
    // LCOV_EXCL_STOP
#endif
    return;
  }
  SNLTruthTable reducedTruthTable =
      reduceTruthTable(uniquifiedCandidate, invTruthTable, std::get<1>(candidate));
  if (reducedTruthTable.size() == 0) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("reducPartialConstantInstance Full constant %s\n",
           unqiuifedCandidate->getModel()->getName().getString().c_str());
// LCOV_EXCL_STOP
#endif
  }
#ifdef DEBUG_PRINTS
  // LCOV_EXCL_START
  printf("reducPartialConstantInstance truth table: %s\n",
         invTruthTable.getString().c_str());
  printf("reducPartialConstantInstance reduced truth table: %s\n",
         reducedTruthTable.getString().c_str());
// LCOV_EXCL_STOP
#endif
  auto result =
      SNLLibraryTruthTables::getDesignForTruthTable(library, reducedTruthTable);
  if (result.first) {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf("reducPartialConstantInstance design %s\n",
           unqiuifedCandidate->getModel()->getName().getString().c_str());
    printf("reducPartialConstantInstance redcued design %s\n",
           result.first->getName().getString().c_str());
// LCOV_EXCL_STOP
#endif
    replaceInstance(uniquifiedCandidate, result);
  } else {
#ifdef DEBUG_PRINTS
    // LCOV_EXCL_START
    printf(
        "reducPartialConstantInstanceNo design found for the reduced truth "
        "table\n");
// LCOV_EXCL_STOP
#endif
  }
}

std::string ReductionOptimization::collectStatistics() const {
  std::stringstream ss;  
  ss << "RO report:" << std::endl;
  for (const auto& entry : reductionStatistics_) {
    ss << entry.first.first  << " -> " << entry.first.second << " : " << entry.second << std::endl;
  }
  return ss.str();
}


