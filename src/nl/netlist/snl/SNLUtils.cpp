// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLUtils.h"

#include <algorithm>
#include <list>
#include <set>

#include "NLLibrary.h"
#include "NLDB0.h"

#include "SNLDesignModeling.h"
#include "SNLDesign.h"

namespace naja::NL {

namespace {

using VisitedDesigns = std::set<const SNLDesign*, SNLDesign::PointerLess>;

void prepareDesignForConcurrentAccess(
    const SNLDesign* design,
    VisitedDesigns& visitedDesigns) {
  if (design == nullptr or visitedDesigns.find(design) != visitedDesigns.end()) {
    return;
  }
  visitedDesigns.insert(design);

  if (design->isPrimitive()) {
    if (NLDB0::isDivMod(design) or NLDB0::isMemory(design)) {
      return;
    }
    for (auto term: design->getBitTerms()) {
      if (term->getDirection() != SNLTerm::Direction::Input) {
        static_cast<void>(
            SNLDesignModeling::getTruthTable(design, term->getOrderID()));
      }
    }
    return;
  }

  for (auto instance: design->getInstances()) {
    prepareDesignForConcurrentAccess(instance->getModel(), visitedDesigns);
  }
}

void collectReachableInstanceCount(
    const SNLDesign* design,
    VisitedDesigns& visitedDesigns,
    VisitedDesigns& activeDesigns,
    SNLUtils::InstanceCount& count) {
  if (design == nullptr or not activeDesigns.insert(design).second) {
    return;
  }
  const bool firstVisit = visitedDesigns.insert(design).second;
  if (firstVisit) {
    ++count.reachableModels;
  }
  for (auto instance: design->getInstances()) {
    ++count.totalInstances;
    if (instance->isLeaf()) {
      ++count.leafInstances;
    }
    if (firstVisit) {
      ++count.foldedTotalInstances;
      if (instance->isLeaf()) {
        ++count.foldedLeafInstances;
      }
    }
    collectReachableInstanceCount(instance->getModel(), visitedDesigns, activeDesigns, count);
  }
  activeDesigns.erase(design);
}

}  // namespace

unsigned SNLUtils::levelize(const SNLDesign* design, DesignsLevel& designsLevel) {
  if (design->getInstances().empty()) {
    designsLevel[design] = 0;
    return 0;
  } else {
    unsigned maxLevel = 0;
    for (auto instance: design->getInstances()) {
      unsigned level = 0;
      auto model = instance->getModel();
      auto it = designsLevel.find(model);
      if (it == designsLevel.end()) {
        level = levelize(model, designsLevel);
        designsLevel[model] = level;
      } else {
        level = it->second;
      }
      if (level > maxLevel) {
        maxLevel = level;
      }
    }
    designsLevel[design] = maxLevel+1;
    return maxLevel+1;
  }
}

void SNLUtils::getDesignsSortedByHierarchicalLevel(const SNLDesign* top, SortedDesigns& sortedDesigns) {
  SNLUtils::DesignsLevel designsLevel;
  SNLUtils::levelize(top, designsLevel);
  sortedDesigns = SortedDesigns(designsLevel.begin(), designsLevel.end());
  std::sort(sortedDesigns.begin(), sortedDesigns.end(),
    [](const DesignLevel& ldl, const DesignLevel& rdl) {
      if (ldl.second == rdl.second) {
        return ldl.first->getNLID() < rdl.first->getNLID();
      }
      return ldl.second < rdl.second;
    }
  );
}

SNLUtils::InstanceCount SNLUtils::countReachableInstances(const SNLDesign* top) {
  InstanceCount count;
  VisitedDesigns visitedDesigns;
  VisitedDesigns activeDesigns;
  collectReachableInstanceCount(top, visitedDesigns, activeDesigns, count);
  return count;
}

NLID::Bit SNLUtils::getWidth(NLID::Bit msb, NLID::Bit lsb) {
  return std::abs(lsb - msb) + 1;
}

SNLDesign* SNLUtils::findTop(const NLLibrary* library) {
  using Tops = std::list<SNLDesign*>;
  Tops tops;
  for (auto design: library->getSNLDesigns()) {
    if (not design->isBlackBox()
      && not design->isPrimitive()
      && design->getSlaveInstances().empty()) {
        tops.push_back(design);
    }
  }
  //Special case: if no top was found accept blackboxes and empty modules
  if (tops.empty()) {
    for (auto design: library->getSNLDesigns()) {
      if (not design->isPrimitive()) {
        tops.push_back(design);
      }
    }
  }
  if (tops.size() == 1) {
    return tops.front();
  }
  return nullptr;
}

void SNLUtils::prepareForConcurrentAccess(const SNLDesign* design) {
  VisitedDesigns visitedDesigns;
  prepareDesignForConcurrentAccess(design, visitedDesigns);
}

std::string SNLUtils::getUnnamedIDString(const char* tag, std::uint64_t id) {
  return "<" + std::string(tag) + ":" + std::to_string(id) + ">";
}

std::string SNLUtils::getNamedString(const NLName& name, const char* tag, std::uint64_t id) {
  if (not name.empty()) {
    return name.getString();
  }
  return getUnnamedIDString(tag, id);
}

std::string SNLUtils::getBusNamedString(
    const NLName& name,
    const char* tag,
    std::uint64_t id,
    NLID::Bit msb,
    NLID::Bit lsb) {
  return getNamedString(name, tag, id) + "["
    + std::to_string(msb) + ":" + std::to_string(lsb) + "]";
}

std::string SNLUtils::getBitNamedString(
    const NLName& name,
    const char* tag,
    std::uint64_t id,
    NLID::Bit bit) {
  return getNamedString(name, tag, id) + "[" + std::to_string(bit) + "]";
}

}  // namespace naja::NL
