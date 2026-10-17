// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NajaException.h"
#include "NajaObject.h"
#include "NajaRelationProperty.h"

using namespace naja;

namespace {

class RelationObject: public NajaObject {
  public:
    using super = NajaObject;
    static RelationObject* create() {
      preCreate();
      RelationObject* object = new RelationObject();
      object->postCreate();
      return object;
    }

    const char* getTypeName() const override { return "RelationObject"; }
    std::string getString() const override { return getTypeName(); }
    std::string getDescription() const override { return getTypeName(); }
};

}  // namespace

TEST(NajaRelationPropertyTest, testOwnerAndMembersShareOneInstance) {
  RelationObject* owner = RelationObject::create();
  RelationObject* member = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");

  EXPECT_EQ(relation, owner->getProperty("rel"));
  EXPECT_EQ(owner, relation->getOwner());
  EXPECT_TRUE(relation->getMembers().empty());

  relation->join(member);
  EXPECT_EQ(relation, member->getProperty("rel"));
  EXPECT_EQ(1u, relation->getMembers().size());
  EXPECT_EQ("rel (owner: RelationObject, 1 members)", relation->getString());

  owner->destroy();
  member->destroy();
}

TEST(NajaRelationPropertyTest, testMembersLeavingKeepsRelation) {
  RelationObject* owner = RelationObject::create();
  RelationObject* a = RelationObject::create();
  RelationObject* b = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");
  relation->join(a);
  relation->join(b);

  relation->leave(a);
  relation->leave(b);

  //Unlike a shared group property, the relation survives its members.
  EXPECT_EQ(relation, owner->getProperty("rel"));
  EXPECT_TRUE(relation->getMembers().empty());
  EXPECT_EQ(nullptr, a->getProperty("rel"));

  owner->destroy();
  a->destroy();
  b->destroy();
}

TEST(NajaRelationPropertyTest, testOwnerDestroyDetachesAllMembers) {
  RelationObject* owner = RelationObject::create();
  RelationObject* a = RelationObject::create();
  RelationObject* b = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");
  relation->join(a);
  relation->join(b);

  owner->destroy();

  EXPECT_EQ(nullptr, a->getProperty("rel"));
  EXPECT_EQ(nullptr, b->getProperty("rel"));

  a->destroy();
  b->destroy();
}

TEST(NajaRelationPropertyTest, testExplicitDestroyDetachesOwnerAndMembers) {
  RelationObject* owner = RelationObject::create();
  RelationObject* member = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");
  relation->join(member);

  relation->destroy();

  EXPECT_EQ(nullptr, owner->getProperty("rel"));
  EXPECT_EQ(nullptr, member->getProperty("rel"));

  owner->destroy();
  member->destroy();
}

TEST(NajaRelationPropertyTest, testMemberDestroyReleasesIt) {
  RelationObject* owner = RelationObject::create();
  RelationObject* member = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");
  relation->join(member);

  member->destroy();

  EXPECT_TRUE(relation->getMembers().empty());
  EXPECT_EQ(relation, owner->getProperty("rel"));

  owner->destroy();
}

TEST(NajaRelationPropertyTest, testOwnerCannotLeave) {
  RelationObject* owner = RelationObject::create();
  NajaRelationProperty* relation = NajaRelationProperty::create(owner, "rel");

  EXPECT_THROW(relation->leave(owner), NajaException);
  EXPECT_EQ(relation, owner->getProperty("rel"));

  owner->destroy();
}
