// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_INST_TERM_H_
#define __PNL_INST_TERM_H_

#include "PNLTerm.h"

namespace naja { namespace NL {

class PNLInstance;
class PNLBitTerm;

class PNLInstTerm final: public PNLNetComponent {
  public:
    friend class PNLInstance;
    friend class PNLBitNet;
    using super = PNLNetComponent;
    PNLInstTerm() = delete;

    PNLDesign* getDesign() const override;

    NLID getNLID() const override;
    /// \return the PNLInstance owning this PNLInstTerm.
    PNLInstance* getInstance() const { return instance_; }
    /// \return the PNLBitTerm referenced by this PNLInstTerm.
    PNLBitTerm* getBitTerm() const { return bitTerm_; }
    PNLBitNet* getNet() const override { return net_; }
    void setNet(PNLNet* net) override;
    
    PNLTerm::Direction getDirection() const override;
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    void destroy() override;

    bool isUnnamed() const override;
    NLName getName() const override;

  private:
    PNLInstTerm(PNLInstance* instance, PNLBitTerm* term);
    static PNLInstTerm* create(PNLInstance* instance, PNLBitTerm* term);
    static void preCreate(const PNLInstance* instance, const PNLBitTerm* term);
    void postCreate();
    void preDestroy() override;
    void destroyFromInstance();

    PNLInstance*  instance_;
    PNLBitTerm*   bitTerm_;
    PNLBitNet*    net_      { nullptr};
};

}} // namespace NL // namespace naja

#endif /* __PNL_INST_TERM_H_ */