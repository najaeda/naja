// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_ATTRIBUTE_H_
#define __SNL_ATTRIBUTE_H_

#include "SNLName.h"

namespace naja { namespace SNL {

class SNLDesign;

class SNLAttribute {
  public:
    static void addAttribute(
      SNLDesign* design,
      const SNLName& name,
      const std::string& value=std::string());
};

}} // namespace SNL // namespace naja

#endif // __SNL_ATTRIBUTE_H_