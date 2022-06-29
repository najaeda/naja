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

#include "SNLID.h"

#include <sstream>

namespace naja { namespace SNL {

#if 0
SNLID SNLID::getMax() {
  return SNLID(
    std::numeric_limits<Type>::max(),
    std::numeric_limits<DBID>::max(),
    std::numeric_limits<LibraryID>::max(),
    std::numeric_limits<DesignID>::max(),
    std::numeric_limits<DesignObjectID>::max(),
    std::numeric_limits<DesignObjectID>::max(),
    std::numeric_limits<Bit>::max());
}
#endif

//LCOV_EXCL_START
std::string SNLID::getString() const {
  std::ostringstream stream;
  stream << "Type:";
  switch (type_) {
    case Type::DB: stream << "DB"; break;
    case Type::Library: stream << "Library"; break;
    case Type::Design: stream << "Design"; break;
    case Type::Term: stream << "Term"; break;
    case Type::TermBit: stream << "TermBit"; break;
    case Type::Net: stream << "Net"; break;
    case Type::NetBit: stream << "NetBit"; break;
    case Type::Instance: stream << "Instance"; break;
    case Type::InstTerm: stream << "InstTerm"; break;
  }
  stream << " ";
  stream << "DBID:" << dbID_ << " ";
  stream << "LibraryID:" << libraryID_ << " ";
  stream << "DesignID:" << designID_ << " ";
  stream << "DesignObjectID:" << designObjectID_ << " "; 
  stream << "InstanceID:" << instanceID_ << " ";
  stream << "Bit:" << bit_ << " ";
  return stream.str();
}
//LCOV_EXCL_STOP

}} // namespace SNL // namespace naja