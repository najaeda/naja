#ifndef __SNL_ID_H_
#define __SNL_ID_H_

#include <tuple>

namespace SNL {

/**
 * \brief The SNLID structure allows global comparison between the various SNL objects and allows to uniquely identify any object. 
 * 
 * The following table details the different fields and their characteristics.
 * 
 * Field       | Type           | Size (bytes) | Max value |
 * ------------|----------------|--------------|-----------|
 * Object type | unsigned char  | 1            | 0-255     |
 * DB          | unsigned char  | 1            | 0-255     |
 * Library     | unsigned short | 2            | 0 - 65535 |
 * Design      | unsigned int   | 4            | 0 - 4,294,967,295 |
 * Instance    | unsigned int   | 4            | 0 - 4,294,967,295 |
 * Net object  | unsigned int   | 4            | 0 - 4,294,967,295 |
 * Bit         | unsigned int   | 4            | 0 - 4,294,967,295 |
 */

struct SNLID final {
  enum class Type {DB, Library, Design, Term, Net, Instance, InstTerm};
  using DBID = unsigned char;
  using LibraryID = unsigned short;
  using DesignID = unsigned int;
  using InstanceID = unsigned int;
  using DesignObjectID =  unsigned int;
  using Bit = int; 

  Type            type_;
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
  SNLID(Type type, DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID id, DesignObjectID instanceID, Bit bit):
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

}

#endif /* __SNL_ID_H_ */
