// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "NLName.h"

namespace naja::NL {

class SNLDesign;
class SNLDesignObject;

struct SNLSourceLoc {
  NLName file;
  std::uint32_t line {0};
  std::uint32_t endLine {0};
  std::uint16_t column {0};
  std::uint16_t endColumn {0};
};

/**
 * Per-object RTL metadata storage.
 *
 * This class stores common source metadata in typed slots and keeps rare
 * key/value string pairs keyed by NLName in an optional map.
 * It is intentionally independent from NLObject ownership mechanics.
 */
class SNLRTLInfos {
  public:
    using InfoName = NLName;
    using InfoValue = std::string;
    using Infos = std::map<InfoName, InfoValue>;

    static SNLRTLInfos* create(SNLDesignObject* designObject);
    static SNLRTLInfos* create(SNLDesign* design);
    void destroy();

    SNLDesignObject* getDesignObject() const { return designObject_; }
    SNLDesign* getDesign() const { return design_; }

    void setSourceLoc(const SNLSourceLoc& sourceLoc);
    bool hasSourceLoc() const { return sourceLoc_.has_value(); }
    const std::optional<SNLSourceLoc>& getSourceLoc() const { return sourceLoc_; }

    void setInfo(const InfoName& name, const InfoValue& value);
    bool hasInfo(const InfoName& name) const;
    InfoValue getInfo(const InfoName& name) const;
    const Infos& getInfos() const;
    std::vector<std::pair<std::string, std::string>> getDumpAttributes() const;
    std::optional<std::pair<std::string, std::string>> getCompactSourceLocAttribute() const;
    void cloneInfos(const SNLRTLInfos& from);

  private:
    static constexpr std::uint32_t kInvalid =
      std::numeric_limits<std::uint32_t>::max();

    explicit SNLRTLInfos(SNLDesignObject* designObject):
      designObject_(designObject) {}
    explicit SNLRTLInfos(SNLDesign* design):
      design_(design) {}
    static void preCreate(SNLDesignObject* designObject);
    static void preCreate(SNLDesign* design);
    void postCreate();
    void preDestroy();

    SNLDesignObject* designObject_ {nullptr};
    SNLDesign* design_ {nullptr};
    std::optional<SNLSourceLoc> sourceLoc_ {};
    std::uint32_t symbolPathId_ {kInvalid};
    std::unique_ptr<Infos> extra_ {};
};

}  // namespace naja::NL
