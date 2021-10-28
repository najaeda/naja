#ifndef __SNL_ID_H_
#define __SNL_ID_H_

#include <tuple>

namespace SNL {

struct SNLID {
  enum class Type {Library, Design};
  using LibraryID = unsigned short;
  using DesignID = unsigned int;
  using DesignObjectID =  unsigned int;
  using BitID = int; 

  Type      type_;
  LibraryID libraryID_  {0};
  DesignID  designID_   {0};

  SNLID(LibraryID libraryID):
    type_(SNLID::Type::Library),
    libraryID_(libraryID)
  {}

  SNLID(LibraryID libraryID, DesignID designID):
    type_(SNLID::Type::Design),
    libraryID_(libraryID),
    designID_(designID)
  {}

  friend bool operator< (const SNLID &lid, const SNLID &rid) {
    return std::make_tuple(lid.type_, lid.libraryID_, lid.designID_)
            < std::make_tuple(rid.type_, rid.libraryID_, rid.designID_);
  }
  

#if 0
  enum  type;
  unsigned short  libraryID_;
  unsigned int    designID_;
  unsigned int 
  size_t  libraryID_
  size_t  designID_
  size_t  objectID_;

  SNLID(unsigned
#endif
  
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
