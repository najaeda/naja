// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLRTLInfos.h"

#include <charconv>
#include <limits>
#include <memory>
#include <system_error>
#include <type_traits>

#include "NLException.h"
#include "SNLDesign.h"
#include "SNLDesignObject.h"

namespace naja::NL {

static_assert(std::is_trivially_copyable_v<SNLSourceLoc>);
static_assert(std::is_standard_layout_v<SNLSourceLoc>);

namespace {

enum class SourceInfoField {
  None,
  File,
  Line,
  Column,
  EndLine,
  EndColumn
};

SourceInfoField getSourceInfoField(const SNLRTLInfos::InfoName& name) {
  const auto nameString = name.getString();
  if (nameString == "sv_src_file") {
    return SourceInfoField::File;
  }
  if (nameString == "sv_src_line") {
    return SourceInfoField::Line;
  }
  if (nameString == "sv_src_column") {
    return SourceInfoField::Column;
  }
  if (nameString == "sv_src_end_line") {
    return SourceInfoField::EndLine;
  }
  if (nameString == "sv_src_end_column") {
    return SourceInfoField::EndColumn;
  }
  return SourceInfoField::None;
}

template<typename T>
T parseUnsignedInfoValue(const std::string& value) {
  std::uint64_t parsed {0};
  const auto* begin = value.data();
  const auto* end = begin + value.size();
  const auto result = std::from_chars(begin, end, parsed);
  if (result.ec != std::errc() || result.ptr != end) {
    return 0;
  }
  constexpr auto max = std::numeric_limits<T>::max();
  if (parsed > max) {
    return max;
  }
  return static_cast<T>(parsed);
}

const SNLRTLInfos::Infos& emptyInfos() {
  static const SNLRTLInfos::Infos infos;
  return infos;
}

}  // namespace

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

void SNLRTLInfos::setSourceLoc(const SNLSourceLoc& sourceLoc) {
  sourceLoc_ = sourceLoc;
}

void SNLRTLInfos::setInfo(const InfoName& name, const InfoValue& value) {
  switch (getSourceInfoField(name)) {
    case SourceInfoField::None:
      if (!extra_) {
        extra_ = std::make_unique<Infos>();
      }
      (*extra_)[name] = value;
      return;
    case SourceInfoField::File:
      if (!sourceLoc_) {
        sourceLoc_.emplace();
      }
      sourceLoc_->file = NLName(value);
      return;
    case SourceInfoField::Line:
      if (!sourceLoc_) {
        sourceLoc_.emplace();
      }
      sourceLoc_->line = parseUnsignedInfoValue<std::uint32_t>(value);
      return;
    case SourceInfoField::Column:
      if (!sourceLoc_) {
        sourceLoc_.emplace();
      }
      sourceLoc_->column = parseUnsignedInfoValue<std::uint16_t>(value);
      return;
    case SourceInfoField::EndLine:
      if (!sourceLoc_) {
        sourceLoc_.emplace();
      }
      sourceLoc_->endLine = parseUnsignedInfoValue<std::uint32_t>(value);
      return;
    case SourceInfoField::EndColumn:
      if (!sourceLoc_) {
        sourceLoc_.emplace();
      }
      sourceLoc_->endColumn = parseUnsignedInfoValue<std::uint16_t>(value);
      return;
  }
}

bool SNLRTLInfos::hasInfo(const InfoName& name) const {
  if (getSourceInfoField(name) != SourceInfoField::None) {
    return sourceLoc_.has_value();
  }
  return extra_ && extra_->find(name) != extra_->end();
}

SNLRTLInfos::InfoValue SNLRTLInfos::getInfo(const InfoName& name) const {
  switch (getSourceInfoField(name)) {
    case SourceInfoField::None:
      if (!extra_) {
        return {};
      }
      if (auto it = extra_->find(name); it != extra_->end()) {
        return it->second;
      }
      return {};
    case SourceInfoField::File:
      return sourceLoc_ ? sourceLoc_->file.getString() : InfoValue {};
    case SourceInfoField::Line:
      return sourceLoc_ ? std::to_string(sourceLoc_->line) : InfoValue {};
    case SourceInfoField::Column:
      return sourceLoc_ ? std::to_string(sourceLoc_->column) : InfoValue {};
    case SourceInfoField::EndLine:
      return sourceLoc_ ? std::to_string(sourceLoc_->endLine) : InfoValue {};
    case SourceInfoField::EndColumn:
      return sourceLoc_ ? std::to_string(sourceLoc_->endColumn) : InfoValue {};
  }
  return {}; // LCOV_EXCL_LINE
}

const SNLRTLInfos::Infos& SNLRTLInfos::getInfos() const {
  return extra_ ? *extra_ : emptyInfos();
}

std::vector<std::pair<std::string, std::string>> SNLRTLInfos::getDumpAttributes() const {
  std::vector<std::pair<std::string, std::string>> attributes;
  attributes.reserve((sourceLoc_ ? 5 : 0) + (extra_ ? extra_->size() : 0));

  if (sourceLoc_) {
    attributes.emplace_back("sv_src_file", sourceLoc_->file.getString());
    attributes.emplace_back("sv_src_line", std::to_string(sourceLoc_->line));
    attributes.emplace_back("sv_src_column", std::to_string(sourceLoc_->column));
    attributes.emplace_back("sv_src_end_line", std::to_string(sourceLoc_->endLine));
    attributes.emplace_back("sv_src_end_column", std::to_string(sourceLoc_->endColumn));
  }
  if (extra_) {
    for (const auto& info : *extra_) {
      attributes.emplace_back(info.first.getString(), info.second);
    }
  }
  return attributes;
}

std::optional<std::pair<std::string, std::string>>
SNLRTLInfos::getCompactSourceLocAttribute() const {
  if (not sourceLoc_) {
    return std::nullopt;
  }
  std::string value = sourceLoc_->file.getString();
  value += ":";
  value += std::to_string(sourceLoc_->line);
  value += ":";
  value += std::to_string(sourceLoc_->column);
  value += "-";
  value += std::to_string(sourceLoc_->endLine);
  value += ":";
  value += std::to_string(sourceLoc_->endColumn);
  return std::make_pair(std::string("naja_sv_src"), std::move(value));
}

void SNLRTLInfos::cloneInfos(const SNLRTLInfos& from) {
  sourceLoc_ = from.sourceLoc_;
  symbolPathId_ = from.symbolPathId_;
  if (from.extra_) {
    extra_ = std::make_unique<Infos>(*from.extra_);
  } else {
    extra_.reset();
  }
}

}  // namespace naja::NL
