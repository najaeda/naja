// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "NLClock.h"

#include <cmath>

#include "NLException.h"
#include "SNLBitTerm.h"
#include "SNLDesign.h"
#include "SNLInstTerm.h"

namespace naja::NL {

namespace {

std::string sourceDescription(const NLObject* source) {
  return source ? source->getDescription() : std::string("null");
}

}  // namespace

NLClock* NLClock::createPrimary(
  SNLDesign* design,
  const NLName& name,
  double period,
  const std::string& timeUnit,
  const std::vector<NLObject*>& sources)
{
  preCreate(design, name);
  checkPeriod(period, timeUnit);
  if (sources.empty()) {
    throw NLException("NLClock " + name.getString() + ": a primary clock needs at least one source");
  }
  for (NLObject* source: sources) {
    checkSource(design, source);
  }

  NLClock* clock = new NLClock(design, name, Kind::Primary);
  clock->period_ = period;
  clock->timeUnit_ = timeUnit;
  clock->sources_ = sources;
  clock->setWaveform(0.0, period / 2.0);
  clock->postCreate();
  return clock;
}

NLClock* NLClock::createGenerated(
  SNLDesign* design,
  const NLName& name,
  NLClock* master,
  NLObject* masterSource,
  unsigned divideBy,
  unsigned multiplyBy,
  bool invert,
  const std::vector<NLObject*>& sources)
{
  preCreate(design, name);
  if (!master) {
    throw NLException("NLClock " + name.getString() + ": a generated clock needs a master clock");
  }
  if (master->getDesign() != design) {
    throw NLException("NLClock " + name.getString() + ": master clock " + master->getName().getString()
      + " belongs to another design");
  }
  checkSource(design, masterSource);
  if (divideBy == 0 || multiplyBy == 0) {
    throw NLException("NLClock " + name.getString() + ": divide and multiply factors must be strictly positive");
  }
  if (sources.empty()) {
    throw NLException("NLClock " + name.getString() + ": a generated clock needs at least one source");
  }
  for (NLObject* source: sources) {
    checkSource(design, source);
  }

  NLClock* clock = new NLClock(design, name, Kind::Generated);
  clock->master_ = master;
  clock->masterSource_ = masterSource;
  clock->divideBy_ = divideBy;
  clock->multiplyBy_ = multiplyBy;
  clock->invert_ = invert;
  clock->timeUnit_ = master->getTimeUnit();
  clock->period_ = master->getPeriod() * divideBy / multiplyBy;
  clock->sources_ = sources;
  clock->setWaveform(0.0, clock->period_ / 2.0);
  clock->postCreate();
  return clock;
}

NLClock::NLClock(SNLDesign* design, const NLName& name, Kind kind):
  name_(name),
  kind_(kind),
  design_(design)
{}

void NLClock::preCreate(const SNLDesign* design, const NLName& name) {
  if (!design) {
    throw NLException("NLClock: design is null");
  }
  if (name.getString().empty()) {
    throw NLException("NLClock: a clock needs a name");
  }
  for (const NLClock* clock: design->getClocks()) {
    if (clock->getName() == name) {
      throw NLException("NLClock " + name.getString() + " already exists in design " + design->getString());
    }
  }
}

void NLClock::postCreate() {
  super::postCreate();
  design_->addClock(this);
}

void NLClock::preDestroy() {
  design_->removeClock(this);
  super::preDestroy();
}

void NLClock::checkSource(const SNLDesign* design, NLObject* source) {
  if (!source) {
    throw NLException("NLClock: source is null");
  }
  if (dynamic_cast<SNLInstTerm*>(source)) {
    throw NLException("NLClock: instance terminal " + sourceDescription(source)
      + " is not supported yet, only top-level terminals");
  }
  auto* term = dynamic_cast<SNLBitTerm*>(source);
  if (!term) {
    throw NLException("NLClock: " + sourceDescription(source) + " is not a top-level terminal");
  }
  if (term->getDesign() != design) {
    throw NLException("NLClock: terminal " + sourceDescription(source) + " belongs to another design");
  }
}

void NLClock::checkPeriod(double period, const std::string& timeUnit) {
  if (!std::isfinite(period) || period <= 0.0) {
    throw NLException("NLClock: period must be a finite, strictly positive value");
  }
  if (timeUnit.empty()) {
    throw NLException("NLClock: time unit is required");
  }
}

void NLClock::setWaveform(double riseAt, double fallAt) {
  if (!std::isfinite(riseAt) || !std::isfinite(fallAt)
      || riseAt < 0.0 || riseAt >= fallAt || fallAt > period_) {
    throw NLException("NLClock " + name_.getString()
      + ": waveform requires 0 <= riseAt < fallAt <= period");
  }
  riseAt_ = riseAt;
  fallAt_ = fallAt;
}

NajaCollection<NLObject*> NLClock::getSources() const {
  return new NajaSTLCollection(&sources_);
}

NLClock* NLClock::getRootClock() {
  NLClock* root = this;
  while (root->master_) {
    root = root->master_;
  }
  return root;
}

bool NLClock::isSynchronousWith(NLClock* other) {
  return other && getRootClock() == other->getRootClock();
}

const char* NLClock::getTypeName() const {
  return "NLClock";
}

std::string NLClock::getString() const {
  return name_.getString();
}

std::string NLClock::getDescription() const {
  return "NLClock " + name_.getString() + " (period " + std::to_string(period_) + " " + timeUnit_ + ")";
}

void NLClock::debugDump(size_t indent, bool recursive, std::ostream& stream) const {
  std::string pad(indent, ' ');
  stream << pad << getDescription() << std::endl;
  stream << pad << "  kind: " << (kind_ == Kind::Primary ? "primary" : "generated") << std::endl;
  stream << pad << "  waveform: " << riseAt_ << " " << fallAt_ << std::endl;
  for (const NLObject* source: sources_) {
    stream << pad << "  source: " << sourceDescription(source) << std::endl;
  }
  if (master_) {
    stream << pad << "  master: " << master_->getString() << " divide " << divideBy_
      << " multiply " << multiplyBy_ << (invert_ ? " inverted" : "") << std::endl;
  }
}

}  // namespace naja::NL
