// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_INST_TERM_H_
#define __SNL_INST_TERM_H_

#include "SNLTerm.h"

namespace naja { namespace SNL {

class SNLInstance;
class SNLBitTerm;

class SNLInstTerm final: public SNLNetComponent {
  public:
    friend class SNLInstance;
    friend class SNLBitNet;
    using super = SNLNetComponent;
    SNLInstTerm() = delete;

    SNLDesign* getDesign() const override;

    SNLID getSNLID() const override;
    SNLInstance* getInstance() const { return instance_; }
    SNLBitTerm* getBitTerm() const { return bitTerm_; }
    SNLBitNet* getNet() const override { return net_; }
    void setNet(SNLNet* net) override;

    bool isAnonymous() const override;
    void setName(const SNLName& name) override;
    
    SNLTerm::Direction getDirection() const override;
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    void destroy() override;
  private:
    SNLInstTerm(SNLInstance* instance, SNLBitTerm* term);
    static SNLInstTerm* create(SNLInstance* instance, SNLBitTerm* term);
    static void preCreate(const SNLInstance* instance, const SNLBitTerm* term);
    void postCreate();
    void preDestroy() override;
    void destroyFromInstance();

    SNLInstance*  instance_;
    SNLBitTerm*   bitTerm_;
    SNLBitNet*    net_      { nullptr};
};

}} // namespace SNL // namespace naja

#endif /* __SNL_INST_TERM_H_ */
