// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0

#include "SNLRTLPrimitives.h"

#include <algorithm>
#include "NLDB0.h"
#include "NLException.h"
#include "SNLBitNet.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLInstance.h"
#include "SNLScalarTerm.h"

namespace naja::NL {

namespace {

bool validNet(SNLDesign* design, SNLNet* net, size_t width) {
  return design && net && net->getDesign() == design &&
    static_cast<size_t>(net->getWidth()) == width;
}

void connectBits(
  SNLInstance* instance, SNLBusTerm* term, const SNLRTLPrimitives::Bits& bits) {
  SNLInstance::Terms terms;
  terms.reserve(bits.size());
  // Canonical mux terminals are [width-1:0].
  for (size_t bit = 0; bit < bits.size(); ++bit) {
    terms.push_back(term->getBit(static_cast<NLID::Bit>(bit)));
  }
  instance->setTermsNets(terms, bits);
}

NLDB0::GateType gateType(SNLRTLPrimitives::GateKind kind) {
  switch (kind) {
    case SNLRTLPrimitives::GateKind::And: return NLDB0::GateType::And;
    case SNLRTLPrimitives::GateKind::Nand: return NLDB0::GateType::Nand;
    case SNLRTLPrimitives::GateKind::Or: return NLDB0::GateType::Or;
    case SNLRTLPrimitives::GateKind::Nor: return NLDB0::GateType::Nor;
    case SNLRTLPrimitives::GateKind::Xor: return NLDB0::GateType::Xor;
    case SNLRTLPrimitives::GateKind::Xnor: return NLDB0::GateType::Xnor;
    case SNLRTLPrimitives::GateKind::Buf: return NLDB0::GateType::Buf;
    case SNLRTLPrimitives::GateKind::Not: return NLDB0::GateType::Not;
  }
  throw NLException("SNLRTLPrimitives::createGate: invalid gate kind");
}

}  // namespace

SNLInstance* SNLRTLPrimitives::createMux(
  SNLDesign* design, SNLBitNet* select,
  const Bits& a, const Bits& b, SNLNet* output) {
  const auto validBits = [design](const Bits& bits) {
    return std::all_of(bits.begin(), bits.end(), [design](auto* bit) {
      return validNet(design, bit, 1);
    });
  };
  if (a.empty() || a.size() != b.size() ||
      !validNet(design, select, 1) || !validNet(design, output, a.size()) ||
      !validBits(a) || !validBits(b)) {
    throw NLException("SNLRTLPrimitives::createMux: invalid nets or widths");
  }
  auto* model = NLDB0::getOrCreateMux2(a.size());
  auto* instance = SNLInstance::create(design, model);
  connectBits(instance, NLDB0::getMux2InputA(model), a);
  connectBits(instance, NLDB0::getMux2InputB(model), b);
  instance->setTermNet(NLDB0::getMux2Select(model), select);
  instance->setTermNet(NLDB0::getMux2Output(model), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createDFF(
  SNLDesign* design, SNLNet* clock, SNLNet* data, SNLNet* output) {
  if (!validNet(design, clock, 1) || !validNet(design, data, 1) ||
      !validNet(design, output, 1)) {
    throw NLException("SNLRTLPrimitives::createDFF: invalid nets or widths");
  }
  auto* instance = SNLInstance::create(design, NLDB0::getDFF());
  instance->setTermNet(NLDB0::getDFFClock(), clock);
  instance->setTermNet(NLDB0::getDFFData(), data);
  instance->setTermNet(NLDB0::getDFFOutput(), output);
  return instance;
}

SNLInstance* SNLRTLPrimitives::createGate(
  SNLDesign* design, GateKind kind,
  const std::vector<SNLNet*>& inputs, SNLNet* output) {
  if (!validNet(design, output, 1) || inputs.empty() ||
      ((kind == GateKind::Buf || kind == GateKind::Not) && inputs.size() != 1) ||
      std::any_of(inputs.begin(), inputs.end(), [design](auto* input) {
        return !validNet(design, input, 1);
      })) {
    throw NLException("SNLRTLPrimitives::createGate: invalid nets, widths, or fan-in");
  }
  const auto type = gateType(kind);
  const bool nOutput = kind == GateKind::Buf || kind == GateKind::Not;
  auto* model = nOutput
      ? NLDB0::getOrCreateNOutputGate(type, 1)
      : NLDB0::getOrCreateNInputGate(type, inputs.size());
  auto* instance = SNLInstance::create(design, model);
  if (nOutput) {
    instance->setTermNet(NLDB0::getGateSingleTerm(model), inputs.front());
    instance->setTermNet(NLDB0::getGateNTerms(model)->getBitAtPosition(0), output);
  } else {
    auto* inputTerms = NLDB0::getGateNTerms(model);
    for (size_t position = 0; position < inputs.size(); ++position)
      instance->setTermNet(inputTerms->getBitAtPosition(position), inputs[position]);
    instance->setTermNet(NLDB0::getGateSingleTerm(model), output);
  }
  return instance;
}

}  // namespace naja::NL
