// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_PYTHON_PROPERTY_H_
#define __NAJA_PYTHON_PROPERTY_H_

#include "NajaPrivateProperty.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace naja {

#define F_PY_OWNED (1 << 0)

class NajaPythonProperty : public NajaPrivateProperty {
  public:
    static const inline std::string Name = "NajaPythonProperty";
    static const std::string& getPropertyName() { return name_; }
    static int getOffset() { return offset_; };
    static void setOffset(int offset);
    static NajaPythonProperty* create(void* shadow = NULL);

    NajaPythonProperty(const NajaPythonProperty&) = delete;
    NajaPythonProperty& operator=(const NajaPythonProperty&) = delete;

    void* getShadow() const { return shadow_; };
    void* getShadowMember() const {
      return (void*)((unsigned long)shadow_ + offset_);
    };

    std::string getName() const override { return getPropertyName(); }
    void onCapturedBy(NajaObject* owner) override;
    void onReleasedBy(const NajaObject* owner) override;
    void onNotOwned();
    std::string getString() const override;

  private:
    static std::string  name_;
    static int          offset_;
    NajaObject*         owner_  {nullptr};
    void*               shadow_ {nullptr};

    NajaPythonProperty() = default;
    NajaPythonProperty(void* shadow);
    void preDestroy() override;
};

#define CHECK_OFFSET(PY_STRUCT, TYPE)                  \
  if (naja::NajaPythonProperty::getOffset() < 0) {       \
    naja::NajaPythonProperty::setOffset(                 \
        (unsigned long)(&(PY_STRUCT->ACCESS_OBJECT)) - \
        (unsigned long)PY_STRUCT);                     \
  }

}  // namespace naja

#endif /* __NAJA_PYTHON_PROPERTY_H_ */