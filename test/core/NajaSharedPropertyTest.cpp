// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "gtest/gtest.h"

#include "NajaObject.h"
#include "NajaSharedProperty.h"

using namespace naja;

namespace {

class GroupObject: public NajaObject {
  public:
    using super = NajaObject;
    static GroupObject* create() {
      preCreate();
      GroupObject* object = new GroupObject();
      object->postCreate();
      return object;
    }

    const char* getTypeName() const override { return "GroupObject"; }
    std::string getString() const override { return getTypeName(); }
    std::string getDescription() const override { return getTypeName(); }
};

}  // namespace

TEST(NajaSharedPropertyTest, testJoinSharesOneInstance) {
  GroupObject* a = GroupObject::create();
  GroupObject* b = GroupObject::create();
  NajaSharedProperty* group = NajaSharedProperty::create("group");

  group->join(a);
  group->join(b);

  EXPECT_EQ(group, a->getProperty("group"));
  EXPECT_EQ(group, b->getProperty("group"));
  EXPECT_EQ(2u, group->getMembers().size());
  EXPECT_EQ("group (2 members)", group->getString());

  a->destroy();
  b->destroy();
}

TEST(NajaSharedPropertyTest, testLeaveKeepsPropertyWhileMembersRemain) {
  GroupObject* a = GroupObject::create();
  GroupObject* b = GroupObject::create();
  NajaSharedProperty* group = NajaSharedProperty::create("group");
  group->join(a);
  group->join(b);

  group->leave(a);

  EXPECT_EQ(nullptr, a->getProperty("group"));
  EXPECT_EQ(group, b->getProperty("group"));
  EXPECT_EQ(1u, group->getMembers().size());

  //Last member leaves: the property is destroyed, so it must not be touched again.
  group->leave(b);
  EXPECT_EQ(nullptr, b->getProperty("group"));

  a->destroy();
  b->destroy();
}

TEST(NajaSharedPropertyTest, testExplicitDestroyDetachesAllMembers) {
  GroupObject* a = GroupObject::create();
  GroupObject* b = GroupObject::create();
  NajaSharedProperty* group = NajaSharedProperty::create("group");
  group->join(a);
  group->join(b);

  group->destroy();

  EXPECT_EQ(nullptr, a->getProperty("group"));
  EXPECT_EQ(nullptr, b->getProperty("group"));

  a->destroy();
  b->destroy();
}

TEST(NajaSharedPropertyTest, testDestroyingMemberReleasesIt) {
  GroupObject* a = GroupObject::create();
  GroupObject* b = GroupObject::create();
  NajaSharedProperty* group = NajaSharedProperty::create("group");
  group->join(a);
  group->join(b);

  a->destroy();

  EXPECT_EQ(1u, group->getMembers().size());
  EXPECT_EQ(group, b->getProperty("group"));

  group->leave(b);
  b->destroy();
}

TEST(NajaSharedPropertyTest, testJoinReplacesPropertyWithSameName) {
  GroupObject* a = GroupObject::create();
  NajaSharedProperty* first = NajaSharedProperty::create("group");
  NajaSharedProperty* second = NajaSharedProperty::create("group");
  first->join(a);

  //Joining a property of the same name replaces the existing one, which
  //loses its only member and is destroyed.
  second->join(a);

  EXPECT_EQ(second, a->getProperty("group"));
  EXPECT_EQ(1u, second->getMembers().size());

  second->destroy();
  a->destroy();
}
