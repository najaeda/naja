/*
 * Copyright 2022 The Naja Authors.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SNL_MACROS_H_
#define __SNL_MACROS_H_

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

#endif // __SNL_MACROS_H_
