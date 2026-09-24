// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLDB0.h"
#include "SNLBitNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstance.h"
#include "SNLInstTerm.h"
#include "SNLScalarTerm.h"

#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace naja::NL::test {
// Evaluate the actual canonical primitive connectivity, with flop outputs
// supplied as the current state. This also catches undriven data and cycles.
inline bool evaluateRTL(SNLBitNet* net, std::unordered_map<SNLBitNet*, bool>& values,
                 std::unordered_set<SNLBitNet*>& visiting) {
  if (net->isConstant0()) return false;
  if (net->isConstant1()) return true;
  if (const auto found = values.find(net); found != values.end()) return found->second;
  if (!visiting.insert(net).second) throw std::runtime_error("combinational cycle");
  SNLInstance* driver = nullptr;
  for (auto* term : net->getInstTerms())
    if (term->getDirection() == SNLTerm::Direction::Output) {
      if (driver) throw std::runtime_error("multiple drivers");
      driver = term->getInstance();
    }
  if (!driver || NLDB0::isDFF(driver->getModel())) throw std::runtime_error("missing data/state");
  auto* model = driver->getModel();
  const auto read = [&](SNLBitTerm* term) {
    return evaluateRTL(driver->getInstTerm(term)->getNet(), values, visiting);
  };
  bool value;
  if (NLDB0::isMux2(model)) {
    value = read(NLDB0::getMux2Select(model))
        ? read(NLDB0::getMux2InputB(model)->getBit(0))
        : read(NLDB0::getMux2InputA(model)->getBit(0));
  } else {
    if (!NLDB0::isGate(model)) throw std::runtime_error("unexpected primitive");
    SNLTruthTable::ConstantInputs inputs;
    for (auto* term : driver->getInstTerms())
      if (term->getDirection() == SNLTerm::Direction::Input)
        inputs.emplace_back(inputs.size(), evaluateRTL(term->getNet(), values, visiting));
    const auto table = NLDB0::getPrimitiveTruthTable(model);
    const auto dependencies = SNLTruthTable::fullDependencies(inputs.size());
    const auto normalized = table.getGenericType() == SNLTruthTable::GenericType::NONE
        ? SNLTruthTable(inputs.size(), static_cast<uint64_t>(table.bits()), dependencies)
        : SNLTruthTable(inputs.size(), table.getGenericType(), dependencies);
    value = normalized.getReducedWithConstants(inputs).all1();
  }
  visiting.erase(net);
  values[net] = value;
  return value;
}

inline std::vector<unsigned> firInputs(unsigned lanes) {
  std::vector<unsigned> inputs;
  const auto mask = (1u << lanes) - 1;
  for (unsigned cycle = 0; cycle < 128; ++cycle)
    inputs.push_back(cycle < 4 ? 0 : cycle < 4 + lanes ? 1u << (cycle - 4) :
        ((cycle * 1777u + 91u) ^ (cycle % 5 ? 0 : mask)) & mask);
  return inputs;
}

// Simulate the actual leaf flop/gate connectivity, then propagate each leaf's
// output through the top-level adder network. Each instance owns its state,
// even when several instances share a specialized model.
inline std::vector<unsigned> simulateFIR(SNLDesign* top, const std::vector<unsigned>& inputs) {
  using Values = std::unordered_map<SNLBitNet*, bool>;
  std::unordered_map<SNLInstance*, Values> states;
  auto* input = top->getBusTerm(NLName("regx_in"));
  auto* output = top->getBusTerm(NLName("y_out"));
  if (!input || !output) throw std::runtime_error("missing FIR ports");
  for (auto* instance : top->getInstances()) {
    if (NLDB0::isGate(instance->getModel()) || NLDB0::isMux2(instance->getModel())) continue;
    auto& state = states[instance];
    for (auto* flop : instance->getModel()->getInstances()) if (NLDB0::isDFF(flop->getModel()))
      state[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] = false;
    if (state.size() != 3) throw std::runtime_error("expected three registers per FIR leaf");
    auto* clock = instance->getModel()->getScalarTerm(NLName("ck"));
    if (!clock || instance->getInstTerm(clock)->getNet() != top->getScalarTerm(NLName("ck"))->getNet())
      throw std::runtime_error("incorrect FIR clock binding");
  }
  if (states.size() != input->getWidth()) throw std::runtime_error("incorrect generated FIR instance count");
  std::vector<unsigned> outputs;
  for (const auto pattern : inputs) {
    Values topValues;
    for (unsigned bit = 0; bit < input->getWidth(); ++bit)
      topValues[input->getBit(bit)->getNet()] = (pattern >> bit) & 1;
    for (auto& [instance, state] : states) {
      auto* model = instance->getModel();
      auto* data = model->getScalarTerm(NLName("reg_in"));
      if (!data) throw std::runtime_error("missing FIR leaf input");
      auto values = state;
      values[data->getNet()] = topValues.at(instance->getInstTerm(data)->getNet());
      std::unordered_set<SNLBitNet*> visiting;
      for (auto* flop : model->getInstances()) if (NLDB0::isDFF(flop->getModel())) {
        if (flop->getInstTerm(NLDB0::getDFFClock())->getNet() != model->getScalarTerm(NLName("ck"))->getNet())
          throw std::runtime_error("incorrect leaf flop clock");
        state[flop->getInstTerm(NLDB0::getDFFOutput())->getNet()] =
            evaluateRTL(flop->getInstTerm(NLDB0::getDFFData())->getNet(), values, visiting);
      }
      values = state;
      values[data->getNet()] = topValues.at(instance->getInstTerm(data)->getNet());
      auto* result = model->getBusTerm(NLName("y_out"));
      if (!result) throw std::runtime_error("missing FIR leaf output");
      for (auto* bit : result->getBits())
        topValues[instance->getInstTerm(bit)->getNet()] = evaluateRTL(bit->getNet(), values, visiting);
    }
    std::unordered_set<SNLBitNet*> visiting;
    unsigned result = 0;
    for (unsigned bit = 0; bit < output->getWidth(); ++bit)
      result |= unsigned(evaluateRTL(output->getBit(bit)->getNet(), topValues, visiting)) << bit;
    outputs.push_back(result);
  }
  return outputs;
}
} // namespace naja::NL::test
