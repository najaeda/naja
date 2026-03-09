// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <map>
#include <string>

#include "NLName.h"

namespace naja::NL {

class SNLDesign;
class SNLDesignObject;

/**
 * Per-object RTL metadata storage.
 *
 * This class stores key/value string pairs keyed by NLName.
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

    void setInfo(const InfoName& name, const InfoValue& value);
    bool hasInfo(const InfoName& name) const;
    InfoValue getInfo(const InfoName& name) const;
    const Infos& getInfos() const;
    void clearInfo(const InfoName& name);
    void clearInfos();
    void cloneInfos(const SNLRTLInfos& from);

  private:
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
    Infos infos_ {};
};

}  // namespace naja::NL
