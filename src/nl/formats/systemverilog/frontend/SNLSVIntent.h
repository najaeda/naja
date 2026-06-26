// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <vector>

#include "SNLRTLInfos.h"

namespace naja::NL {

class NLObject;
class SNLDesignObject;

struct SNLSVIntentEnumMember {
  std::string name {};
  std::string encoding {};
};

struct SNLSVIntentType {
  bool valid {false};
  std::string typeName {};
  std::string canonicalKind {};
  SNLSourceLoc declLoc {NLName()};
  bool isEnum {false};
  unsigned enumWidth {0};
  SNLSourceLoc enumDeclLoc {NLName()};
  std::vector<SNLSVIntentEnumMember> members {};
};

struct SNLSVIntentParam {
  std::string name {};
  std::string value {};
  std::string expr {};
  bool localParam {false};
  SNLSourceLoc loc {NLName()};
};

struct SNLSVIntentParams {
  bool valid {false};
  std::string module {};
  std::vector<SNLSVIntentParam> params {};
};

class SNLSVIntent {
  public:
    static bool available();
    static SNLSVIntentType typeOf(const SNLDesignObject* object);
    static SNLSVIntentType typeOf(const NLObject* object);
    static SNLSVIntentParams parametersOf(const SNLDesignObject* object);
    static SNLSVIntentParams parametersOf(const NLObject* object);
    static SNLSVIntentType packageMemberType(
      const std::string& package,
      const std::string& member);
    static SNLSVIntentParam packageMember(
      const std::string& package,
      const std::string& member);
};

}  // namespace naja::NL
