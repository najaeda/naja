// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <string>

#include "NajaProperty.h"

namespace naja {

/// \brief a named property linking an owner object to a set of member objects.
///
/// The owner holds the relation under its name, and members hold it too, so
/// every participant reaches the same instance through getProperty(name).
/// Unlike NajaSharedProperty, the relation lives as long as its owner: members
/// may join and leave freely without destroying it, while destroying the owner
/// (or the relation itself) detaches every member.
class NajaRelationProperty: public NajaProperty {
  public:
    using super = NajaProperty;
    using Members = std::set<const NajaObject*>;

    /// \brief create a relation owned by owner. The relation dies with owner.
    /// \param owner the owner of this NajaRelationProperty.
    /// \param name the name of this NajaRelationProperty.
    /// \return created NajaRelationProperty.
    static NajaRelationProperty* create(NajaObject* owner, const std::string& name);

    std::string getName() const override { return name_; }
    std::string getString() const override;

    /// \return the owner of this NajaRelationProperty.
    NajaObject* getOwner() const { return owner_; }
    /// \return the members of this NajaRelationProperty, excluding the owner.
    const Members& getMembers() const { return members_; }

    /// \brief add object as a member of the relation, replacing any property of the same name.
    void join(NajaObject* object);
    /// \brief remove object from the members. The relation itself is kept.
    /// \throws NajaException if object is the owner.
    void leave(NajaObject* object);

  protected:
    NajaRelationProperty(const std::string& name, NajaObject* owner): name_(name), owner_(owner) {}

    void preDestroy() override;
    void onCapturedBy(NajaObject* object) override;
    void onReleasedBy(const NajaObject* object) override;

  private:
    std::string     name_;
    NajaObject*     owner_ {nullptr};
    Members         members_;
};

} // namespace naja
