// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <vector>

namespace naja::NL {

class NLObject;
class SNLBitNet;
class SNLDesign;
class SNLNet;

/**
 * Source-language-neutral construction of width-explicit SNL circuits.
 *
 * Bit vectors are ordered least-significant bit first. Frontends retain
 * ownership of parsing, type resolution, and source metadata. Hooks let a
 * frontend reuse its constant and unary-gate caches and annotate every object
 * created by this builder without introducing a dependency on its source IR.
 */
class SNLDesignBuilder {
public:
  using Bits = std::vector<SNLBitNet*>;
  using ConstantProvider = std::function<SNLBitNet*(bool)>;
  using NotProvider = std::function<SNLBitNet*(SNLBitNet*)>;
  using ObjectCreated = std::function<void(NLObject*)>;

  struct Hooks {
    ConstantProvider getConstant{};
    NotProvider createNot{};
    ObjectCreated objectCreated{};
  };

  explicit SNLDesignBuilder(SNLDesign* design);
  SNLDesignBuilder(SNLDesign* design, Hooks hooks);

  bool add(const Bits& left, const Bits& right, Bits& result);
  bool subtract(const Bits& left, const Bits& right, Bits& result);

  bool mux(SNLBitNet* select, const Bits& input0, const Bits& input1,
           Bits& result, SNLNet* output = nullptr);

  static Bits collectBits(SNLNet* net);

private:
  bool validBits(const Bits& bits) const;
  SNLBitNet* getConstant(bool one);
  SNLBitNet* createNot(SNLBitNet* input);
  bool createFullAdder(SNLBitNet* inputA, SNLBitNet* inputB, SNLBitNet* carryIn,
                       SNLBitNet* sum, SNLBitNet* carryOut);
  bool connectTermBits(class SNLInstance* instance, class SNLTerm* term,
                       const Bits& bits) const;
  void notifyCreated(NLObject* object) const;

  SNLDesign* design_{nullptr};
  Hooks hooks_{};
};

}  // namespace naja::NL
