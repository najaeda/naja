// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "SNLTerm.h"

namespace naja::NL {

class SNLScalarTerm;
class SNLBusTerm;

class SNLBundleTerm final: public SNLTerm {
  public:
    friend class SNLDesign;
    friend class SNLScalarTerm;
    friend class SNLBusTerm;
    using super = SNLTerm;

    static SNLBundleTerm* create(
      SNLDesign* design,
      Direction direction,
      const NLName& name=NLName());

    static SNLBundleTerm* create(
      SNLDesign* design,
      NLID::DesignObjectID id,
      Direction direction,
      const NLName& name=NLName());

    SNLDesign* getDesign() const override { return design_; }
    NLID getNLID() const override;
    NLID::DesignObjectID getID() const override { return id_; }
    size_t getFlatID() const override { return flatID_; }
    NLName getName() const override { return name_; }
    bool isUnnamed() const override { return name_.empty(); }
    void setName(const NLName& name) override;

    SNLBitNet* getNet() const override { return nullptr; }
    void setNet(SNLNet* net) override;

    NLID::Bit getWidth() const override;
    NajaCollection<SNLBitTerm*> getBits() const override;
    SNLTerm::Direction getDirection() const override { return direction_; }

    size_t getNumMembers() const { return members_.size(); }
    SNLTerm* getMember(size_t index) const;
    NajaCollection<SNLTerm*> getMembers() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const SNLNetComponent* other, std::string& reason) const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

  private:
    using Members = std::vector<SNLTerm*>;

    SNLBundleTerm(SNLDesign* design, Direction direction, const NLName& name);
    SNLBundleTerm(
      SNLDesign* design,
      NLID::DesignObjectID id,
      Direction direction,
      const NLName& name);

    static void preCreate(const SNLDesign* design, const NLName& name);
    static void preCreate(const SNLDesign* design, NLID::DesignObjectID id, const NLName& name);

    void postCreateAndSetID();
    void postCreate() override;
    void commonPreDestroy();
    void destroyFromDesign() override;
    SNLTerm* clone(SNLDesign* design) const override;
    void preDestroy() override;
    void addMember(SNLTerm* member);
    void removeMember(SNLTerm* member);
    void rebuildBits();

    void setID(NLID::DesignObjectID id) override { id_ = id; }
    void setFlatID(size_t flatID) override { flatID_ = flatID; }

    SNLDesign*           design_      {nullptr};
    NLID::DesignObjectID id_          {0};
    size_t               flatID_      {0};
    NLName               name_        {};
    SNLTerm::Direction   direction_;
    Members              members_     {};
    std::vector<SNLBitTerm*> bitTerms_{};
};

}  // namespace naja::NL
