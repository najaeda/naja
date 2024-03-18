// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_TERM_H_
#define __SNL_BIT_TERM_H_

#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLBitTerm: public SNLTerm {
  public:
    friend class SNLBitNet;
    using super = SNLTerm;
    
    //Allows to compare SNLBitTerms in the same design.
    struct InDesignLess {
      bool operator()(const SNLBitTerm* lt, const SNLBitTerm* rt) const {
        return std::make_pair(lt->getID(), lt->getBit()) < std::make_pair(rt->getID(), rt->getBit());
      }
    };

    SNLID::Bit getSize() const override { return 1; }
    virtual SNLID::Bit getBit() const = 0;
    SNLBitNet* getNet() const override { return net_; }
    void setNet(SNLNet* net) override;
  protected:
    SNLBitTerm() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;
  private:
    SNLBitNet*    net_  { nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_BIT_TERM_H_