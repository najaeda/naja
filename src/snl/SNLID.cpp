#include "SNLID.h"

#include <string>
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
