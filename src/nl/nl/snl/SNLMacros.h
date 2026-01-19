// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0


#pragma once
#include <sstream>

//
//Contains various methods used in different while similar... contexts
//Might seem dirty to use good old macros but I prefer this than plain copy/paste...
//Did not find a proper way to implement this with templates.
//

#define NET_COMPONENT_SET_NET(TYPE) \
void TYPE::setNet(SNLNet* net) { \
  if (net and net->getDesign() not_eq getDesign()) { \
    std::string reason = "Impossible setNet call with incompatible designs: "; \
    reason += getString() + " is in " + getDesign()->getString() + " while "; \
    reason += net->getString() + " is in " + net->getDesign()->getString(); \
    throw NLException(reason); \
  } \
  SNLBitNet* bitNet = nullptr; \
  if (net) { \
    bitNet = dynamic_cast<SNLBitNet*>(net); \
    if (not bitNet) { \
      auto bus = static_cast<SNLBusNet*>(net); \
      if (bus) { \
        if (bus->getWidth() != 1) { \
          std::ostringstream reason; \
          reason << "In design " << getDesign()->getString() << ", cannot set " << getString(); \
          reason << " to " << net->getString() << ", bus width is " << bus->getWidth(); \
          throw NLException(reason.str()); \
        } \
        bitNet = bus->getBit(bus->getMSB()); \
      } \
    } \
  } \
  if (net_ not_eq bitNet) { \
    if (net_) { \
      net_->removeComponent(this); \
    } \
    net_ = bitNet; \
    if (net_) { \
      net_->addComponent(this); \
    } \
  } \
}

#define PNL_NET_COMPONENT_SET_NET(TYPE) \
void TYPE::setNet(PNLNet* net) { \
  if (net and net->getDesign() not_eq getDesign()) { \
    std::string reason = "Impossible setNet call with incompatible designs: "; \
    reason += getString() + " is in " + getDesign()->getString() + " while "; \
    reason += net->getString() + " is in " + net->getDesign()->getString(); \
    throw NLException(reason); \
  } \
  PNLBitNet* bitNet = nullptr; \
  if (net) { \
    bitNet = dynamic_cast<PNLBitNet*>(net); \
    if (not bitNet) { \
      assert(false); \
    } \
  } \
  if (net_ not_eq bitNet) { \
    if (net_) { \
      net_->removeComponent(this); \
    } \
    net_ = bitNet; \
    if (net_) { \
      net_->addComponent(this); \
    } \
  } \
}

//
//Contains various methods used in different while similar... contexts
//Might seem dirty to use good old macros but I prefer this than plain copy/paste...
//Did not find a proper way to implement this with templates.
//

#define PNL_NET_COMPONENT_SET_NET(TYPE) \
void TYPE::setNet(PNLNet* net) { \
  if (net and net->getDesign() not_eq getDesign()) { \
    std::string reason = "Impossible setNet call with incompatible designs: "; \
    reason += getString() + " is in " + getDesign()->getString() + " while "; \
    reason += net->getString() + " is in " + net->getDesign()->getString(); \
    throw NLException(reason); \
  } \
  PNLBitNet* bitNet = nullptr; \
  if (net) { \
    bitNet = dynamic_cast<PNLBitNet*>(net); \
    if (not bitNet) { \
    assert(false); \
    } \
  } \
  if (net_ not_eq bitNet) { \
    if (net_) { \
      net_->removeComponent(this); \
    } \
    net_ = bitNet; \
    if (net_) { \
      net_->addComponent(this); \
    } \
  } \
}

#define DEEP_COMPARE_MEMBER(MEMBER, REF, OTHER) \
{ \
  auto ref##MEMBER##It = REF->get##MEMBER().begin(); \
  auto other##MEMBER##It = OTHER->get##MEMBER().begin(); \
  while (ref##MEMBER##It not_eq REF->get##MEMBER().end()) { \
    if (other##MEMBER##It == OTHER->get##MEMBER().end()) { \
      reason += "In " + REF->getDescription() + ", different size of " #MEMBER + ":" ; \
      reason += std::to_string(REF->get##MEMBER().size()) + " vs "; \
      reason += std::to_string(OTHER->get##MEMBER().size()); \
      return false; \
    } \
    auto ref##MEMBER = *ref##MEMBER##It; \
    auto other##MEMBER = *other##MEMBER##It; \
    if (not ref##MEMBER->deepCompare(other##MEMBER, reason)) { \
      return false; \
    } \
    ++ref##MEMBER##It; \
    ++other##MEMBER##It; \
  } \
  if (other##MEMBER##It not_eq OTHER->get##MEMBER().end()) { \
    return false; \
  } \
}

#define DESIGN_OBJECT_SET_NAME(TYPE, METHOD_TYPE, STRING) \
void TYPE::setName(const NLName& name) { \
  if (name_ == name) { \
    return; \
  } \
  if (not name.empty()) { \
    /* check collision */ \
    if (auto collision = design_->get##METHOD_TYPE(name)) { \
      std::ostringstream reason; \
      reason << "In design " << design_->getString() \
        << ", cannot rename " << getString() << " to " \
        << name.getString() << ", another #STRING: " << collision->getString() \
        << " has already this name."; \
      throw NLException(reason.str()); \
    } \
  } \
  auto previousName = getName(); \
  name_ = name; \
  getDesign()->rename(this, previousName); \
}

#define OWNER_RENAME(OWNER, TYPE, MAP) \
void OWNER::rename(TYPE* object, const NLName& previousName) { \
  /*if object was unnamed, and new one is not... just insert with new name */ \
  if (previousName.empty()) { \
    if (not object->isUnnamed()) { \
      /* collision is already verified by object before trying insertion */ \
      MAP[object->getName()] = object->getID(); \
    } /* else nothing to do, unnamed to unnamed */ \
  } else { \
    auto node = MAP.extract(previousName); \
    assert(node); \
    if (not object->isUnnamed()) { \
      node.key() = object->getName(); \
      MAP.insert(std::move(node)); \
    } \
  } \
}

