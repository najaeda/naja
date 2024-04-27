// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_ID_H_
#define __SNL_ID_H_

#include <string>
#include <tuple>
#include <cstdint>

namespace naja { namespace SNL {

/**
 * \brief The SNLID structure allows global comparison between the various SNL objects
 * and allows to uniquely identify any object. 
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
  using DesignObjectID  = uint32_t;
  using Bit             = int32_t;

  struct DBDesignReference {
    LibraryID libraryID_;
    DesignID  designID_;
    DBDesignReference() = delete;
    DBDesignReference(const DBDesignReference&) = default;
    DBDesignReference(LibraryID libraryID, DesignID designID):
      libraryID_(libraryID),
      designID_(designID)
    {}

    friend bool operator==(const DBDesignReference& lid, const DBDesignReference& rid) {
      return std::tie(lid.libraryID_, lid.designID_) == std::tie(rid.libraryID_, rid.designID_);
    }
    friend bool operator!=(const DBDesignReference& lid, const DBDesignReference& rid) {
      return not (lid == rid);
    }
  };

  /// \brief DesignReference is a structure allowing to reference uniquely a SNLDesign. 
  struct DesignReference {
    DBID              dbID_;
    LibraryID         libraryID_;
    DesignID          designID_;

    DesignReference() = delete;
    DesignReference(const DesignReference&) = default;
    DesignReference(const SNLID& id):
      dbID_(id.dbID_),
      libraryID_(id.libraryID_),
      designID_(id.designID_)
    {}
      
    DesignReference(DBID dbID, LibraryID libraryID, DesignID designID):
      dbID_(dbID),
      libraryID_(libraryID),
      designID_(designID)
    {}
    DBDesignReference getDBDesignReference() const {
      return DBDesignReference(libraryID_, designID_);
    }
    std::string getString() const {
      return "[db:" + std::to_string(dbID_)
        + " lib:" + std::to_string(libraryID_)
        + " design:" + std::to_string(designID_)
        + "]";
    }
    friend bool operator==(const DesignReference& lid, const DesignReference& rid) {
      return std::tie(lid.dbID_, lid.libraryID_, lid.designID_) ==
        std::tie(rid.dbID_, rid.libraryID_, rid.designID_);
    }
    friend bool operator!=(const DesignReference& lid, const DesignReference& rid) {
      return not (lid == rid);
    }
  };

  struct DesignObjectReference {
    DBID              dbID_;
    LibraryID         libraryID_;
    DesignID          designID_;
    DesignObjectID    designObjectID_;

    DesignObjectReference() = delete;
    DesignObjectReference(const DesignObjectReference&) = default;
    DesignObjectReference(const SNLID& id):
      dbID_(id.dbID_),
      libraryID_(id.libraryID_),
      designID_(id.designID_),
      designObjectID_(id.designObjectID_)
    {}
    DesignObjectReference(DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID designObjectID):
      dbID_(dbID),
      libraryID_(libraryID),
      designID_(designID),
      designObjectID_(designObjectID)
    {}
    DesignObjectReference(const DesignReference& designReference, DesignObjectID designObjectID):
      DesignObjectReference(
        designReference.dbID_,
        designReference.libraryID_,
        designReference.designID_,
        designObjectID)
    {}
    DesignReference getDesignReference() const {
      return DesignReference(dbID_, libraryID_, designID_);
    }
    std::string getString() const {
      return "[db:" + std::to_string(dbID_)
        + " lib:" + std::to_string(libraryID_)
        + " design:" + std::to_string(designID_)
        + " object:" + std::to_string(designObjectID_)
        + "]";
    }
    friend bool operator==(const DesignObjectReference& lid, const DesignObjectReference& rid) {
      return std::tie(lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_) ==
        std::tie(rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_);
    }
    friend bool operator!=(const DesignObjectReference& lid, const DesignObjectReference& rid) {
      return not (lid == rid);
    }
  };

  struct BitNetReference {
    bool              isBusBit_       {false};
    DBID              dbID_           {0};
    LibraryID         libraryID_      {0};
    DesignID          designID_       {0};
    DesignObjectID    designObjectID_ {0};
    Bit               bit_            {0};

    BitNetReference() = default;
    BitNetReference(const BitNetReference&) = default;
    //ScalarNet
    BitNetReference(const DesignReference& designReference, DesignObjectID designObjectID):
      dbID_(designReference.dbID_),
      libraryID_(designReference.libraryID_),
      designID_(designReference.designID_),
      designObjectID_(designObjectID)
    {}
    //ScalarNet
    BitNetReference(DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID designObjectID):
      dbID_(dbID),
      libraryID_(libraryID),
      designID_(designID),
      designObjectID_(designObjectID)
    {}
    //BusNetBit
    BitNetReference(DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID designObjectID, Bit bit):
      isBusBit_(true),
      dbID_(dbID),
      libraryID_(libraryID),
      designID_(designID),
      designObjectID_(designObjectID),
      bit_(bit)
    {}
    ///General constructor
    BitNetReference(bool isBusBit, DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID designObjectID, Bit bit):
      isBusBit_(isBusBit),
      dbID_(dbID),
      libraryID_(libraryID),
      designID_(designID),
      designObjectID_(designObjectID),
      bit_(bit)
    {}
    ///BusNetBit reference
    BitNetReference(const DesignReference& designReference, DesignObjectID designObjectID, Bit bit):
      BitNetReference(
        designReference.dbID_,
        designReference.libraryID_,
        designReference.designID_,
        designObjectID,
        bit)
    {}

    DesignReference getDesignReference() const {
      return DesignReference(dbID_, libraryID_, designID_);
    }
    std::string getString() const {
      std::string str("[db:" + std::to_string(dbID_));
      str += " lib:" + std::to_string(libraryID_);
      str += " design:" + std::to_string(designID_);
      str += " object:" + std::to_string(designObjectID_);
      if (isBusBit_) {
        str += " bit: " + std::to_string(bit_); 
      }
      str += "]";
      return str;
    }
    friend bool operator==(const BitNetReference& lid, const BitNetReference& rid) {
      return std::tie(lid.isBusBit_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.bit_) ==
        std::tie(rid.isBusBit_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, lid.bit_);
    }
    friend bool operator!=(const BitNetReference& lid, const BitNetReference& rid) {
      return not (lid == rid);
    }
  };
  

  Type            type_           {0};
  DBID            dbID_           {0};
  LibraryID       libraryID_      {0};
  DesignID        designID_       {0};
  DesignObjectID  designObjectID_ {0};
  DesignObjectID  instanceID_     {0};
  Bit             bit_            {0};           

  SNLID() = delete;
  
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

  friend bool operator<(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            < std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator<=(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            <= std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator==(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            == std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator!=(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            != std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator>(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            > std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }

  friend bool operator>=(const SNLID& lid, const SNLID& rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_, lid.instanceID_, lid.bit_)
            >= std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_, rid.instanceID_, rid.bit_);
  }


//LCOV_EXCL_START
  std::string getString() const {
    std::string str("[Type: "); 
    switch (type_) {
      case Type::DB: str += "DB"; break;
      case Type::Library: str += "Library"; break;
      case Type::Design: str += "Design"; break;
      case Type::Term: str += "Term"; break;
      case Type::TermBit: str += "TermBit"; break;
      case Type::Net: str += "Net"; break;
      case Type::NetBit: str += "NetBit"; break;
      case Type::Instance: str += "Instance"; break;
      case Type::InstTerm: str += "InstTerm"; break;
    }
    str += " db:" + std::to_string(dbID_);
    str += " lib:" + std::to_string(libraryID_);
    str += " design:" + std::to_string(designID_);
    str += " object:" + std::to_string(designObjectID_);
    str += " instance:" + std::to_string(instanceID_);
    str += " bit:" + std::to_string(bit_);
    str += "]";
    return str;
  }
  //LCOV_EXCL_STOP
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