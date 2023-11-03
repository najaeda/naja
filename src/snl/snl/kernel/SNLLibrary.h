// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_LIBRARY_H_
#define __SNL_LIBRARY_H_

#include <map>
#include "NajaCollection.h"
#include "SNLDesign.h"

namespace naja { namespace SNL {

class SNLDB;

class SNLLibrary final: public SNLObject {
  public:
    friend class SNLDB;
    friend class SNLDesign;
    using super = SNLObject;

    class Type {
      public:
        enum TypeEnum {
          Standard, InDB0, Primitives
        };
        Type(const TypeEnum& typeEnum);
        Type(const Type& type) = default;
        operator const TypeEnum&() const {return typeEnum_;}
        std::string getString() const;
        private:
          TypeEnum typeEnum_;
    };

    SNLLibrary() = delete;
    SNLLibrary(const SNLLibrary& library) = delete;

    static SNLLibrary* create(SNLDB* db, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLDB* db, Type type, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLDB* db, SNLID::LibraryID id, Type type, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, Type type, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, SNLID::LibraryID id, Type type, const SNLName& name = SNLName());

    bool isRoot() const { return isRoot_; }

    SNLDB* getDB() const;

    ///\return parent SNLLibrary
    SNLLibrary* getParentLibrary() const;
    ///\return child SNLLibrary with SNLID::LibraryID id
    SNLLibrary* getLibrary(SNLID::LibraryID id) const;
    ///\return child SNLLibrary named name
    SNLLibrary* getLibrary(const SNLName& name) const;
    ///\return the collection of sub SNLLibrary
    NajaCollection<SNLLibrary*> getLibraries() const;
    ///\return SNLDesign with SNLID::DesignID id
    SNLDesign* getDesign(SNLID::DesignID id) const;
    ///\return SNLDesign named name
    SNLDesign* getDesign(const SNLName& name) const;
    ///\return the collection of SNLDesign contained in this SNLLibrary
    NajaCollection<SNLDesign*> getDesigns() const;

    SNLID::LibraryID getID() const { return id_; }
    SNLID getSNLID() const;
    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

    Type getType() const { return type_; }
    bool isStandard() const { return type_ == Type::Standard; }
    bool isInDB0() const { return type_ == Type::InDB0; }
    bool isPrimitives() const { return type_ == Type::Primitives; }

    void mergeAssigns();

    friend bool operator< (const SNLLibrary &ll, const SNLLibrary &rl) {
      return ll.getSNLID() < rl.getSNLID();
    }

    bool deepCompare(const SNLLibrary* library, std::string& reason) const;
  private:
    static void preCreate(SNLDB* db, Type type, const SNLName& name);
    static void preCreate(SNLDB* db, SNLID::LibraryID id, const Type type, const SNLName& name);
    static void preCreate(SNLLibrary* parent, Type type, const SNLName& name);
    static void preCreate(SNLLibrary* parent, SNLID::LibraryID id, Type type, const SNLName& name);
    void destroyFromDB();
    void destroyFromParentLibrary();
    void postCreate();
    void postCreateAndSetID();
    void commonPreDestroy();
    void preDestroy() override;

    SNLLibrary(SNLDB* db, Type type, const SNLName& name);
    SNLLibrary(SNLDB* db, SNLID::LibraryID libraryID, Type type, const SNLName& name);
    SNLLibrary(SNLLibrary* parent, Type type, const SNLName& name);
    SNLLibrary(SNLLibrary* parent, SNLID::LibraryID libraryID, Type type, const SNLName& name);

    void addLibrary(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);
    void addDesignAndSetID(SNLDesign* design);
    void addDesign(SNLDesign* design);
    void removeDesign(SNLDesign* design);

    using SNLLibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    using SNLLibraryDesignsHook =
      boost::intrusive::member_hook<SNLDesign, boost::intrusive::set_member_hook<>, &SNLDesign::libraryDesignsHook_>;
    using SNLLibraryDesigns = boost::intrusive::set<SNLDesign, SNLLibraryDesignsHook>;
    using SNLDesignNameIDMap = std::map<SNLName, SNLID::DesignID>;

    SNLID::LibraryID                    id_;
    SNLName                             name_                 {};
    Type                                type_                 { Type::Standard };
    void*                               parent_               { nullptr };
    bool                                isRoot_               { false };
    boost::intrusive::set_member_hook<> dbLibrariesHook_      {};
    boost::intrusive::set_member_hook<> libraryLibrariesHook_ {};
    using SNLLibraryLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::libraryLibrariesHook_>;
    using SNLLibraryLibraries = boost::intrusive::set<SNLLibrary, SNLLibraryLibrariesHook>;
    SNLLibraryLibraries                 libraries_            {}; 
    SNLLibraryNameIDMap                 libraryNameIDMap_     {};
    SNLLibraryDesigns                   designs_              {};
    SNLDesignNameIDMap                  designNameIDMap_      {};
};

}} // namespace SNL // namespace naja

#endif // __SNL_LIBRARY_H_
