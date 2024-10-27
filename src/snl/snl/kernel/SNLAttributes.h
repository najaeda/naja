// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_ATTRIBUTES_H_
#define __SNL_ATTRIBUTES_H_

#include "SNLName.h"

namespace naja { namespace SNL {

class SNLDesign;

class SNLAttributes {
  public:
    class SNLAttribute {
      public:
        SNLAttribute(const SNLName& name, const std::string& value);
        SNLName getName() const { return name_; }
        std::string getValue() const { return value_; }
        bool hasValue() const { return not value_.empty(); }
      private:
        SNLName     name_;
        std::string value_;
    };
    static void addAttribute(SNLDesign* design, const SNLAttribute& attribute);
};

}} // namespace SNL // namespace naja

#endif // __SNL_ATTRIBUTES_H_