// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLRTLInfos.h"

#include "NLException.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"

namespace naja::NL {

SNLRTLInfos* SNLRTLInfos::create(SNLDesignObject* designObject) {
  preCreate(designObject);
  auto rtlInfos = new SNLRTLInfos(designObject);
  rtlInfos->postCreate();
  return rtlInfos;
}

SNLRTLInfos* SNLRTLInfos::create(SNLDesign* design) {
  preCreate(design);
  auto rtlInfos = new SNLRTLInfos(design);
  rtlInfos->postCreate();
  return rtlInfos;
}

void SNLRTLInfos::destroy() {
  preDestroy();
  delete this;
}

void SNLRTLInfos::preCreate(SNLDesignObject* designObject) {
  if (!designObject) {
    throw NLException("Cannot create SNLRTLInfos with null SNLDesignObject");
  }
  if (designObject->rtlInfos_) {
    std::string reason = "SNLDesignObject ";
    reason += designObject->getDescription();
    reason += " already has SNLRTLInfos";
    throw NLException(reason);
  }
}

void SNLRTLInfos::preCreate(SNLDesign* design) {
  if (!design) {
    throw NLException("Cannot create SNLRTLInfos with null SNLDesign");
  }
  if (design->rtlInfos_) {
    std::string reason = "SNLDesign ";
    reason += design->getDescription();
    reason += " already has SNLRTLInfos";
    throw NLException(reason);
  }
}

void SNLRTLInfos::postCreate() {
  if (designObject_) {
    designObject_->rtlInfos_ = this;
  }
  if (design_) {
    design_->rtlInfos_ = this;
  }
}

void SNLRTLInfos::preDestroy() {
  if (designObject_) {
    designObject_->rtlInfos_ = nullptr;
    designObject_ = nullptr;
  }
  if (design_) {
    design_->rtlInfos_ = nullptr;
    design_ = nullptr;
  }
}

void SNLRTLInfos::setInfo(const InfoName& name, const InfoValue& value) {
  infos_[name] = value;
}

bool SNLRTLInfos::hasInfo(const InfoName& name) const {
  return infos_.find(name) != infos_.end();
}

SNLRTLInfos::InfoValue SNLRTLInfos::getInfo(const InfoName& name) const {
  auto it = infos_.find(name);
  if (it == infos_.end()) {
    return {};
  }
  return it->second;
}

const SNLRTLInfos::Infos& SNLRTLInfos::getInfos() const {
  return infos_;
}

void SNLRTLInfos::cloneInfos(const SNLRTLInfos& from) {
  infos_ = from.infos_;
}

}  // namespace naja::NL
