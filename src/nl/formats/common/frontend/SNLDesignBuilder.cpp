// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLDesignBuilder.h"

#include <utility>

#include "NLDB0.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLBusNet.h"
#include "SNLBusNetBit.h"
#include "SNLBusTerm.h"
#include "SNLBusTermBit.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLScalarNet.h"
#include "SNLScalarTerm.h"
#include "SNLTerm.h"

namespace naja::NL {

SNLDesignBuilder::SNLDesignBuilder(SNLDesign* design)
    : SNLDesignBuilder(design, Hooks{}) {}

SNLDesignBuilder::SNLDesignBuilder(SNLDesign* design, Hooks hooks)
    : design_(design), hooks_(std::move(hooks)) {}

void SNLDesignBuilder::notifyCreated(NLObject* object) const {
  if (object && hooks_.objectCreated) {
    hooks_.objectCreated(object);
  }
}

bool SNLDesignBuilder::validBits(const Bits& bits) const {
  if (!design_ || bits.empty()) {
    return false;
  }
  for (auto* bit : bits) {
    if (!bit || bit->getDesign() != design_) {
      return false;
    }
  }
  return true;
}

SNLBitNet* SNLDesignBuilder::getConstant(bool one) {
  if (!design_) {
    return nullptr;
  }
  if (hooks_.getConstant) {
    auto* constant = hooks_.getConstant(one);
    return constant && constant->getDesign() == design_ ? constant : nullptr;
  }

  const auto type = one ? SNLNet::Type::Assign1 : SNLNet::Type::Assign0;
  for (auto* bit : design_->getBitNets()) {
    if (bit->getType() == type) {
      return bit;
    }
  }
  auto* constant = SNLScalarNet::create(design_);
  constant->setType(type);
  notifyCreated(constant);
  return constant;
}

SNLBitNet* SNLDesignBuilder::createNot(SNLBitNet* input) {
  if (!input || input->getDesign() != design_) {
    return nullptr;
  }
  if (hooks_.createNot) {
    auto* output = hooks_.createNot(input);
    return output && output->getDesign() == design_ ? output : nullptr;
  }

  auto* model =
      NLDB0::getOrCreateNOutputGate(NLDB0::GateType(NLDB0::GateType::Not), 1);
  if (!model) {
    return nullptr;
  }
  auto* inputTerm = NLDB0::getGateSingleTerm(model);
  auto* outputTerms = NLDB0::getGateNTerms(model);
  if (!inputTerm || !outputTerms) {
    return nullptr;
  }
  auto* outputTerm = outputTerms->getBitAtPosition(0);
  if (!outputTerm) {
    return nullptr;
  }

  auto* output = SNLScalarNet::create(design_);
  auto* instance = SNLInstance::create(design_, model);
  notifyCreated(output);
  notifyCreated(instance);
  instance->setTermNet(inputTerm, input);
  instance->setTermNet(outputTerm, output);
  return output;
}

bool SNLDesignBuilder::createFullAdder(SNLBitNet* inputA, SNLBitNet* inputB,
                                       SNLBitNet* carryIn, SNLBitNet* sum,
                                       SNLBitNet* carryOut) {
  if (!inputA || !inputB || !carryIn || !sum || !carryOut ||
      inputA->getDesign() != design_ || inputB->getDesign() != design_ ||
      carryIn->getDesign() != design_ || sum->getDesign() != design_ ||
      carryOut->getDesign() != design_) {
    return false;
  }
  auto* model = NLDB0::getFA();
  if (!model) {
    return false;
  }
  auto* instance = SNLInstance::create(design_, model);
  notifyCreated(instance);
  instance->setTermNet(NLDB0::getFAInputA(), inputA);
  instance->setTermNet(NLDB0::getFAInputB(), inputB);
  instance->setTermNet(NLDB0::getFAInputCI(), carryIn);
  instance->setTermNet(NLDB0::getFAOutputS(), sum);
  instance->setTermNet(NLDB0::getFAOutputCO(), carryOut);
  return true;
}

bool SNLDesignBuilder::add(const Bits& left, const Bits& right, Bits& result) {
  result.clear();
  if (left.size() != right.size() || !validBits(left) || !validBits(right)) {
    return false;
  }
  auto* carry = getConstant(false);
  if (!carry) {
    return false;
  }

  result.reserve(left.size());
  for (size_t bit = 0; bit < left.size(); ++bit) {
    auto* sum = SNLScalarNet::create(design_);
    auto* carryOut = SNLScalarNet::create(design_);
    notifyCreated(sum);
    notifyCreated(carryOut);
    if (!createFullAdder(left[bit], right[bit], carry, sum, carryOut)) {
      return false;
    }
    result.push_back(sum);
    carry = carryOut;
  }
  return true;
}

bool SNLDesignBuilder::subtract(const Bits& left, const Bits& right,
                                Bits& result) {
  result.clear();
  if (left.size() != right.size() || !validBits(left) || !validBits(right)) {
    return false;
  }
  auto* zero = getConstant(false);
  auto* one = getConstant(true);
  if (!zero || !one) {
    return false;
  }

  Bits invertedRight;
  invertedRight.reserve(right.size());
  for (auto* bit : right) {
    if (bit == zero) {
      invertedRight.push_back(one);
    } else if (bit == one) {
      invertedRight.push_back(zero);
    } else {
      auto* inverted = createNot(bit);
      if (!inverted) {
        return false;
      }
      invertedRight.push_back(inverted);
    }
  }

  auto* carry = one;
  result.reserve(left.size());
  for (size_t bit = 0; bit < left.size(); ++bit) {
    auto* difference = SNLScalarNet::create(design_);
    auto* carryOut = SNLScalarNet::create(design_);
    notifyCreated(difference);
    notifyCreated(carryOut);
    if (!createFullAdder(left[bit], invertedRight[bit], carry, difference,
                         carryOut)) {
      return false;
    }
    result.push_back(difference);
    carry = carryOut;
  }
  return true;
}

SNLDesignBuilder::Bits SNLDesignBuilder::collectBits(SNLNet* net) {
  if (!net) {
    return {};
  }
  if (auto* scalar = dynamic_cast<SNLScalarNet*>(net)) {
    return {scalar};
  }
  auto* bus = dynamic_cast<SNLBusNet*>(net);
  if (!bus) {
    return {};
  }

  Bits bits;
  bits.reserve(bus->getWidth());
  const auto msb = bus->getMSB();
  const auto lsb = bus->getLSB();
  const auto step = lsb <= msb ? 1 : -1;
  for (auto bit = lsb; bit != msb + step; bit += step) {
    if (auto* busBit = bus->getBit(bit)) {
      bits.push_back(busBit);
    }
  }
  return bits;
}

bool SNLDesignBuilder::connectTermBits(SNLInstance* instance, SNLTerm* term,
                                       const Bits& bits) const {
  if (!instance || !term ||
      bits.size() != static_cast<size_t>(term->getWidth())) {
    return false;
  }

  SNLInstance::Terms orderedTerms;
  orderedTerms.reserve(bits.size());
  if (auto* bus = dynamic_cast<SNLBusTerm*>(term)) {
    const auto step = bus->getMSB() >= bus->getLSB() ? 1 : -1;
    for (size_t bit = 0; bit < bits.size(); ++bit) {
      orderedTerms.push_back(
          bus->getBit(bus->getLSB() + step * static_cast<NLID::Bit>(bit)));
    }
  } else if (bits.size() == 1) {
    orderedTerms.push_back(dynamic_cast<SNLBitTerm*>(term));
  }
  if (orderedTerms.size() != bits.size() || !orderedTerms.front()) {
    return false;
  }
  instance->setTermsNets(orderedTerms, bits);
  return true;
}

bool SNLDesignBuilder::mux(SNLBitNet* select, const Bits& input0,
                           const Bits& input1, Bits& result, SNLNet* output) {
  result.clear();
  if (!select || select->getDesign() != design_ ||
      input0.size() != input1.size() || !validBits(input0) ||
      !validBits(input1)) {
    return false;
  }
  if (output && (output->getDesign() != design_ ||
                 static_cast<size_t>(output->getWidth()) != input0.size())) {
    return false;
  }

  if (!output) {
    if (input0.size() == 1) {
      output = SNLScalarNet::create(design_);
    } else {
      output = SNLBusNet::create(design_,
                                 static_cast<NLID::Bit>(input0.size() - 1), 0);
    }
    notifyCreated(output);
  }

  auto* model = NLDB0::getOrCreateMux2(input0.size());
  if (!model) {
    return false;
  }
  auto* instance = SNLInstance::create(design_, model);
  notifyCreated(instance);
  if (!connectTermBits(instance, NLDB0::getMux2InputA(model), input0) ||
      !connectTermBits(instance, NLDB0::getMux2InputB(model), input1)) {
    return false;
  }
  instance->setTermNet(NLDB0::getMux2Select(model), select);
  instance->setTermNet(NLDB0::getMux2Output(model), output);
  result = collectBits(output);
  return result.size() == input0.size();
}

}  // namespace naja::NL
