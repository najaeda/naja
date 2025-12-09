// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLEquipotential.h"

#include "SNLPath.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include <sstream>  

namespace {

using namespace naja::NL;

struct SNLEquipotentialExtractor {
  explicit SNLEquipotentialExtractor(
    SNLEquipotential::InstTermOccurrences& instTermOccurrences,
    SNLEquipotential::Terms& terms,
    SNLNet::Type& type
  ): instTermOccurrences_(instTermOccurrences),
    terms_(terms),
    type_(type)
  {}

  void extractNetComponentsFromNetOccurrence(
    const naja::NL::SNLOccurrence& netOccurrence,
    const SNLOccurrence& comingFromComponent,
    bool start
  ) {
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      //already visited... report loop ??
      return;
    }
    visitedNetOccurrences_.insert(netOccurrence);
    auto net = dynamic_cast<const SNLBitNet*>(netOccurrence.getObject());
    if (net->getType() != SNLNet::Type::Standard) {
      type_ = net->getType();
    }
    auto path = netOccurrence.getPath();
    for (auto component: net->getComponents()) {
      if (not start and component == comingFromComponent.getObject()) {
        continue;
      }
      if (auto instTerm = dynamic_cast<SNLInstTerm*>(component)) {
        auto instance = instTerm->getInstance();
        if (instance->isLeaf()) {
          //construct InstTerm Occurrences
          instTermOccurrences_.emplace(naja::NL::SNLOccurrence(path, instTerm));
        } else {
          //get inside instance by exploring from term occurrence
          auto term = instTerm->getBitTerm();
          auto instancePath = naja::NL::SNLPath(path, instance);
          //
          extractNetFromNetComponentOccurrence(naja::NL::SNLOccurrence(instancePath, term), false);
        }
      } else {
        auto term = static_cast<SNLBitTerm*>(component);
        if (path.empty()) {
          //top term: end exploration here
          terms_.insert(term);      
        } else {
          for (auto term: net->getBitTerms()) {
            //go up: extract from upper instance term occurrence
            auto parentPath = path.getHeadPath();
            auto instance = path.getTailInstance();
            auto instTerm = instance->getInstTerm(term);
            //
            extractNetFromNetComponentOccurrence(naja::NL::SNLOccurrence(parentPath, instTerm), false);
          }
        }
      }
    }
  }
  
  //start extraction from net component
  //construct net and start exploration
  void extractNetFromNetComponentOccurrence(
    const naja::NL::SNLOccurrence& netComponentOccurrence,
    bool start
  ) {
    auto netOccurrence = netComponentOccurrence.getComponentBitNetOccurrence();
    if (netOccurrence.isValid()) {
      extractNetComponentsFromNetOccurrence(netOccurrence, netComponentOccurrence, start);
    }
  } 

#if 0
    auto netOccurrence = netComponentOccurrence.getNetOccurrence();
    if (visitedNetOccurrences_.find(netOccurrence) != visitedNetOccurrences_.end()) {
      auto net = netOccurrence.getNet();
      auto path = netOccurrence.getPath();
      for (auto component: net->getComponents()) {
        auto instTerm = dynamic_cast<SNLInstTerm*>(component);
        if (instTerm) {

        } else {
          auto term = static_cast<SNLBitTerm*>(component);
          if (path.empty()) {
            
          } else {
            //construct term occurrence and go up
            auto termOccurrence = SNLBitTermOccurrence(path.getHeadPath(), term);
            extractFromNetComponentOccurrence(termOccurrence);
          }
        }
      }
    }
    naja::SNL::SNLPath path = netComponentOccurrence.getPath();
    naja::SNL::SNLBitNet* net = netComponentOccurrence.getNet();

    if (net) {
      //explore NetOccurrence
      naja::SNL::SNLBitNetOccurrence netOccurrence(path, net);
      extractFromNetOccurrence(netOccurrence);
    }
  }
#endif

  private:
    SNLEquipotential::InstTermOccurrences&  instTermOccurrences_;
    SNLEquipotential::Terms&                terms_;
    SNLNet::Type&                           type_;
    using NetOccurrences = std::set<SNLOccurrence>;
    NetOccurrences                          visitedNetOccurrences_  {};
};

}

namespace naja { namespace NL {

SNLEquipotential::SNLEquipotential(SNLNetComponent* netComponent):
  SNLEquipotential(SNLOccurrence(netComponent))
{}

SNLEquipotential::SNLEquipotential(const SNLOccurrence& netComponentOccurrence) {
  SNLEquipotentialExtractor extractor(instTermOccurrences_, terms_, type_);
  extractor.extractNetFromNetComponentOccurrence(netComponentOccurrence, true);
}

std::string SNLEquipotential::getString() const {
  std::ostringstream stream;
  stream << "SNLEquipotential: " << type_.getString() << ", ";
  stream << "InstTermOccurrences: [";
  bool first = true;
  for (const auto& instTermOccurrence: instTermOccurrences_) {
    if (not first) {
      stream << ", ";
    }
    first = false;
    stream << instTermOccurrence.getString();
  }
  stream << "], Terms: [";
  first = true;
  for (const auto& term: terms_) {
    if (not first) {
      stream << ", ";
    }
    first = false;
    stream << term->getString();
  }
  stream << "]";
  return stream.str();
}

NajaCollection<SNLBitTerm*> SNLEquipotential::getTerms() const {
  return NajaCollection(new NajaSTLCollection(&terms_));
}

NajaCollection<SNLOccurrence> SNLEquipotential::getInstTermOccurrences() const {
  return NajaCollection(new NajaSTLCollection(&instTermOccurrences_));
}

}} // namespace NL // namespace naja
