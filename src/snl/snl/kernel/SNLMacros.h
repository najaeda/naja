// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_MACROS_H_
#define __SNL_MACROS_H_

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
    throw SNLException(reason); \
  } \
  SNLBitNet* bitNet = nullptr; \
  if (net) { \
    bitNet = dynamic_cast<SNLBitNet*>(net); \
    if (not bitNet) { \
      std::string reason = "Impossible setNet call with incompatible nets size: "; \
      throw SNLException(reason); \
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

#define DEEP_COMPARE_MEMBER(MEMBER) \
{ \
  auto this##MEMBER##It = get##MEMBER().begin(); \
  auto other##MEMBER##It = other->get##MEMBER().begin(); \
  while (this##MEMBER##It not_eq get##MEMBER().end()) { \
    if (other##MEMBER##It == other->get##MEMBER().end()) { \
      reason += "In " + getDescription() + ", different size of " #MEMBER + ":" ; \
      reason += std::to_string(get##MEMBER().size()) + " vs "; \
      reason += std::to_string(other->get##MEMBER().size()); \
      return false; \
    } \
    auto this##MEMBER = *this##MEMBER##It; \
    auto other##MEMBER = *other##MEMBER##It; \
    if (not this##MEMBER->deepCompare(other##MEMBER, reason)) { \
      return false; \
    } \
    ++this##MEMBER##It; \
    ++other##MEMBER##It; \
  } \
  if (other##MEMBER##It not_eq other->get##MEMBER().end()) { \
    return false; \
  } \
}

#define DESIGN_OBJECT_SET_NAME(TYPE, METHOD_TYPE, STRING) \
void TYPE::setName(const SNLName& name) { \
  if (name_ == name) { \
    return; \
  } \
  if (not name.empty()) { \
    /* check collision */ \
    if (auto collision = getDesign()->get##METHOD_TYPE(name)) { \
      std::ostringstream reason; \
      reason << "In design " << getDesign()->getString() \
        << ", cannot rename " << getString() << " to " \
        << name.getString() << ", another #STRING: " << collision->getString() \
        << " has already this name."; \
      throw SNLException(reason.str()); \
    } \
  } \
  auto previousName = getName(); \
  name_ = name; \
  getDesign()->rename(this, previousName); \
}

#define DESIGN_RENAME(TYPE, METHOD_TYPE, MAP) \
void SNLDesign::rename(TYPE* object, const SNLName& previousName) { \
  /*if object was anonymous, and new one is not... just insert with new name */ \
  if (previousName.empty()) { \
    if (not object->isAnonymous()) { \
      /* collision is already verified by object before trying insertion */ \
      MAP[object->getName()] = object->getID(); \
    } /* else nothing to do, anonymous to anonymous */ \
  } else { \
    auto node = MAP.extract(previousName); \
    assert(node); \
    if (not object->isAnonymous()) { \
      node.key() = object->getName(); \
      MAP.insert(std::move(node)); \
    } \
  } \
}

#endif // __SNL_MACROS_H_
