#ifndef __SNL_ID_H_
#define __SNL_ID_H_

#include <tuple>

namespace SNL {

struct SNLID {
  enum class Type {DB, Library, Design, Term, Net, Instance};
  using DBID = unsigned char;
  using LibraryID = unsigned short;
  using DesignID = unsigned int;
  using InstanceID = unsigned int;
  using DesignObjectID =  unsigned int;
  using BitID = int; 

  Type            type_;
  DBID            dbID_           {0};
  LibraryID       libraryID_      {0};
  DesignID        designID_       {0};
  DesignObjectID  designObjectID_ {0};

  SNLID(DBID dbID):
    type_(SNLID::Type::DB),
    dbID_(dbID)
  {}

  SNLID(DBID dbID, LibraryID libraryID):
    type_(SNLID::Type::Library),
    dbID_(dbID),
    libraryID_(libraryID)
  {}

  SNLID(DBID dbID, LibraryID libraryID, DesignID designID):
    type_(SNLID::Type::Design),
    dbID_(dbID),
    libraryID_(libraryID),
    designID_(designID)
  {}

  SNLID(Type type, DBID dbID, LibraryID libraryID, DesignID designID, DesignObjectID id):
    type_(type),
    dbID_(dbID),
    libraryID_(libraryID),
    designID_(designID),
    designObjectID_(id)
  {}

  friend bool operator< (const SNLID &lid, const SNLID &rid) {
    return std::tie(lid.type_, lid.dbID_, lid.libraryID_, lid.designID_, lid.designObjectID_)
            < std::tie(rid.type_, rid.dbID_, rid.libraryID_, rid.designID_, rid.designObjectID_);
  }
  
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
