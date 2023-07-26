// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SCALAR_NET_H_
#define __SNL_SCALAR_NET_H_

#include "SNLBitNet.h"
#include "SNLName.h"

namespace naja { namespace SNL {

class SNLScalarNet final: public SNLBitNet {
  public:
    friend class SNLDesign;
    using super = SNLBitNet;

    static SNLScalarNet* create(SNLDesign* design, const SNLName& name=SNLName());
    static SNLScalarNet* create(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name=SNLName());

    SNLDesign* getDesign() const override { return design_; }

    SNLID getSNLID() const override;
    SNLID::DesignObjectID getID() const override { return id_; }

    SNLName getName() const override { return name_; }
    bool isAnonymous() const override { return name_.empty(); }
    void setName(const SNLName& name);
    NajaCollection<SNLBitNet*> getBits() const override;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, std::ostream& stream = std::cerr) const override;
  private:
    SNLScalarNet(SNLDesign* design, const SNLName& name);
    SNLScalarNet(SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name);
    static void preCreate(const SNLDesign* design, const SNLName& name);
    static void preCreate(const SNLDesign* design, SNLID::DesignObjectID id, const SNLName& name);
    void postCreate();
    void postCreateAndSetID();
    void destroyFromDesign() override;
    void commonPreDestroy();
    void preDestroy() override;

    void setID(SNLID::DesignObjectID id) override { id_ = id; }

    SNLDesign*                          design_;
    SNLID::DesignObjectID               id_;
    SNLName                             name_;
};

}} // namespace SNL // namespace naja

#endif // __SNL_SCALAR_NET_H_
