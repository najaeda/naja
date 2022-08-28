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

#ifndef __SNL_ID_H_
#define __SNL_ID_H_

#include <tuple>
#include <string>

namespace naja { namespace SNL {

/**
 * \brief The SNLID structure allows global comparison between the various SNL objects and allows to uniquely identify any object. 
 * 
 * The following table details the different fields and their characteristics.
 * 
 * Field       | Type      | Size (bytes) | Max value |
 * ------------|-----------|--------------|-----------|
 * Object type | uint8_t   | 1            | 0-255     |
 * DB          | uint8_t   | 1            | 0-255     |
 * Library     | uint16_t  | 2            | 0 - 65535 |
 * Design      | uint32_t  | 4            | 0 - 4,294,967,295 |
 * Instance    | uint32_t  | 4            | 0 - 4,294,967,295 |
 * Net object  | uint32_t  | 4            | 0 - 4,294,967,295 |
 * Bit         | int32_t   | 4            | 0 - 4,294,967,295 |
 */

struct SNLID final {
  enum class Type:        uint8_t {DB=1, Library, Design, Term, TermBit, Net, NetBit, Instance, InstTerm};
  using DBID            = uint8_t;
  using LibraryID       = uint16_t;
  using DesignID        = uint32_t;
  using InstanceID      = uint32_t;
  using DesignObjectID  = uint32_t;
  using Bit             = int32_t;

  struct DBDesignReference {
    LibraryID libraryID_  {0};
    DesignID  designID_   {0};
    DBDesignReference() = delete;
    DBDesignReference(const DBDesignReference&) = default;
    DBDesignReference(LibraryID libraryID, DesignID designID):
      libraryID_(libraryID),
      designID_(designID)
    {}
  };

  struct UniverseDesignReference {
    DBID        dbID_                     {0};
    DBDesignReference dbDesignReference_  {0, 0};

    UniverseDesignReference() = delete;
    UniverseDesignReference(const UniverseDesignReference&) = default;
    UniverseDesignReference(DBID dbID, LibraryID libraryID, DesignID designID):
      dbID_(dbID),
      dbDesignReference_(libraryID, designID)
    {}
    DBDesignReference getDBDesignReference() const {
      return dbDesignReference_;
    }
  };

  Type            type_           {0};
  DBID            dbID_           {0};
  LibraryID       libraryID_      {0};
  DesignID        designID_       {0};
  DesignObjectID  designObjectID_ {0};
  DesignObjectID  instanceID_     {0};
  Bit             bit_            {0};           

  SNLID() = default;
  
  ///Special constructor for SNLDB
  SNLID(DBID dbID):
    type_(SNLID::Type::DB),
    dbID_(dbID)
  {}

  ///Special constructor for SNLLibrary
  SNLID(DBID dbID, LibraryID libraryID):
    type_(SNLID::Type::Library),
    dbID_(dbID),
    libraryID_(libraryID)
  {}

  ///Special constructor for SNLDesign
  SNLID(DBID dbID, LibraryID libraryID, DesignID designID):
    type_(SNLID::Type::Design),
    dbID_(dbID),
    libraryID_(libraryID),
    designID_(designID)
  {}

  ///Special constructor for SNLDesignObject
  constexpr SNLID(Type type, DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID id, DesignObjectID instanceID, Bit bit):
    type_(type),
    dbID_(dbID),
    libraryID_(libraryID),
    designID_(designID),
    designObjectID_(id),
    instanceID_(instanceID),
    bit_(bit)
  {}

  friend bool operator< (const SNLID &lid, const SNLID &rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            < std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator== (const SNLID &lid, const SNLID &rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            == std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  std::string getString() const;
};

template<typename T>
struct SNLIDComp {
  bool operator()(const SNL::SNLID& id, const T& obj) const {
    return id < obj.getSNLID();
  }
  bool operator()(const T& obj, const SNL::SNLID& id) const {
    return obj.getSNLID() < id;
  }
};

}} // namespace SNL // namespace naja

#endif // __SNL_ID_H_
