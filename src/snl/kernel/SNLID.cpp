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

namespace SNL {

std::string SNLID::getString() const {
  std::ostringstream stream;
  stream << "Type:";
  switch (type_) {
    case Type::DB: stream << "DB"; break;
    case Type::Library: stream << "Library"; break;
    case Type::Design: stream << "Design"; break;
    case Type::Term: stream << "Term"; break;
    case Type::Net: stream << "Net"; break;
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

}
