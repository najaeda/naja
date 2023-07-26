// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_BIT_TERM_H_
#define __SNL_BIT_TERM_H_

#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLBitTerm: public SNLTerm {
  public:
    using super = SNLTerm;

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