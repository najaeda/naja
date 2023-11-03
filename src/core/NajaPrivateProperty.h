// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_PRIVATE_PROPERTY_H_
#define __NAJA_PRIVATE_PROPERTY_H_

#include "NajaProperty.h"

namespace naja {

class NajaPrivateProperty: public NajaProperty {
  public:
    using super = NajaProperty;

    NajaObject* getOwner() const { return owner_; }
  protected:
    NajaPrivateProperty() = default;
    static void preCreate(const NajaObject* object, const std::string& name);
    void postCreate(NajaObject* owner);
    void preDestroy() override;
    void onCapturedBy(NajaObject* object) override;
    void onReleasedBy(const NajaObject* object) override;
  private:
    NajaObject* owner_  {nullptr};
};

} // namespace naja

#endif /* __NAJA_PRIVATE_PROPERTY_H_ */