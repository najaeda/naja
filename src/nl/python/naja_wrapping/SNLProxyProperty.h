// Copyright 2024 The Naja Authors.
// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NAJA_DUMPABLE_PROPERTY_H_
#define __NAJA_DUMPABLE_PROPERTY_H_

#include "NajaPrivateProperty.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace naja {

#define F_PY_OWNED (1 << 0)

class SNLProxyProperty : public NajaPrivateProperty {
 public:
  static const std::string& getPropertyName() { return _name; }
  static int getOffset() { return _offset; };
  static void setOffset(int offset);
  static SNLProxyProperty* create(void* shadow = NULL);

 public:
  void* getShadow() const { return _shadow; };
  void* getShadowMember() const {
    return (void*)((unsigned long)_shadow + _offset);
  };

 public:
  std::string getName() const override { return getPropertyName(); }
  void onCapturedBy(NajaObject* owner) override;
  void onReleasedBy(const NajaObject* owner) override;
  void onNotOwned();

 public:
  virtual std::string getString() const override;

 protected:
  static std::string _name;
  static int _offset;
  NajaObject* _owner;
  void* _shadow;

 protected:
  SNLProxyProperty(void* _shadow);
  void preDestroy() override;

 private:
  SNLProxyProperty(const SNLProxyProperty&);
  SNLProxyProperty& operator=(const SNLProxyProperty&);
};

#define CHECK_OFFSET(PY_STRUCT, TYPE)                  \
  if (naja::SNLProxyProperty::getOffset() < 0) {       \
    naja::SNLProxyProperty::setOffset(                 \
        (unsigned long)(&(PY_STRUCT->ACCESS_OBJECT)) - \
        (unsigned long)PY_STRUCT);                     \
  }

}  // namespace naja

#endif /* __NAJA_DUMPABLE_PROPERTY_H_ */
