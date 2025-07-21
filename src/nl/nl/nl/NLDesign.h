// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "NLID.h"

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

    template<typename T>
      struct CompareByID {
      bool operator()(const T& lt, const T& rt) const {
        return lt.getID() < rt.getID();
      }
      bool operator()(NLID::DesignObjectID id, const T& obj) const {
        return id < obj.getID();
      }
      bool operator()(const T& obj, NLID::DesignObjectID id) const {
        return obj.getID() < id;
      }
    };
};

}} // namespace NL // namespace naja