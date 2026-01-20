// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include "NajaPrivateProperty.h"

#include <cstdint>
#include <cstddef>

namespace naja {

class NajaPythonProperty : public NajaPrivateProperty {
  public:
    static const inline std::string Name = "NajaPythonProperty";
    static const std::string& getPropertyName() { return name_; }
    static std::ptrdiff_t getOffset() { return offset_; };
    static void setOffset(std::ptrdiff_t offset);
    static NajaPythonProperty* create(void* shadow = NULL);

    NajaPythonProperty(const NajaPythonProperty&) = delete;
    NajaPythonProperty& operator=(const NajaPythonProperty&) = delete;

    void* getShadow() const { return shadow_; };
    void* getShadowMember() const {
      if (!shadow_ || offset_ < 0) return nullptr;
      auto* base = reinterpret_cast<std::byte*>(shadow_);
      return reinterpret_cast<void*>(base + static_cast<std::ptrdiff_t>(offset_));
    };

    std::string getName() const override { return getPropertyName(); }
    void onCapturedBy(NajaObject* owner) override;
    void onReleasedBy(const NajaObject* owner) override;
    void onNotOwned();
    std::string getString() const override;

  private:
    static std::string    name_;
    static std::ptrdiff_t offset_;
    NajaObject*           owner_  {nullptr};
    void*                 shadow_ {nullptr};

    NajaPythonProperty() = default;
    NajaPythonProperty(void* shadow);
    void preDestroy() override;
};

#define CHECK_OFFSET(PY_STRUCT, TYPE) \
  if (naja::NajaPythonProperty::getOffset() < 0) { \
    auto* base = reinterpret_cast<std::byte*>(PY_STRUCT); \
    auto* member = reinterpret_cast<std::byte*>(&(PY_STRUCT->ACCESS_OBJECT)); \
    naja::NajaPythonProperty::setOffset(static_cast<std::ptrdiff_t>(member - base)); \
  }

}  // namespace naja

