// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __NL_DESIGN_H_
#define __NL_DESIGN_H_

#include <map>
#include <string>

namespace naja { namespace NL {

class NLDesign {
  public:
    class CompareType {
      public:
        enum CompareTypeEnum {
          Complete, IgnoreID, IgnoreIDAndName
        };
        CompareType(const CompareTypeEnum& typeEnum);
        CompareType(const CompareType& type) = default;
        operator const CompareTypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          CompareTypeEnum typeEnum_;
    };
};

}} // namespace NL // namespace naja

#endif // __NL_DESIGN_H_