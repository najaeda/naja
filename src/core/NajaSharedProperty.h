// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <set>
#include <string>

#include "NajaProperty.h"

namespace naja {

/// \brief a named property shared by a group of NajaObjects.
///
/// Every member sees the same instance through NajaObject::getProperty(name).
/// The property lives while at least one member holds it: when the last member
/// leaves it is destroyed, the same lifetime rule as NajaPrivateProperty.
/// Destroying it explicitly detaches it from every member first.
class NajaSharedProperty: public NajaProperty {
  public:
    using super = NajaProperty;
    using Members = std::set<const NajaObject*>;

    static NajaSharedProperty* create(const std::string& name);

    std::string getName() const override { return name_; }
    std::string getString() const override;

    /// \brief add object as a member of the group, replacing any property of the same name.
    void join(NajaObject* object);
    /// \brief remove object from the members. The group is destroyed when its last member
    /// leaves, so this property must not be used afterwards in that case.
    void leave(NajaObject* object);

    /// \return the members of this NajaSharedProperty.
    const Members& getMembers() const { return members_; }

  protected:
    explicit NajaSharedProperty(const std::string& name): name_(name) {}

    void preDestroy() override;
    void onCapturedBy(NajaObject* object) override;
    void onReleasedBy(const NajaObject* object) override;

  private:
    std::string name_;
    Members     members_;
};

} // namespace naja
