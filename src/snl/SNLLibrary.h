#ifndef __SNL_LIBRARY_H_
#define __SNL_LIBRARY_H_

#include <map>
#include "SNLDesign.h"

namespace SNL {

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
    static SNLLibrary* create(SNLDB* db, const Type& type, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, const Type& type, const SNLName& name = SNLName());

    bool isRootLibrary() const { return isRootLibrary_; }

    SNLDB* getDB() const;

    SNLCollection<SNLLibrary*> getLibraries() const;
    SNLLibrary* getParentLibrary() const;
    SNLLibrary* getLibrary(SNLID::LibraryID id);
    SNLLibrary* getLibrary(const SNLName& name);

    SNLCollection<SNLDesign*> getDesigns() const;
    SNLDesign* getDesign(SNLID::DesignID id);
    SNLDesign* getDesign(const SNLName& name);

    SNLID::LibraryID getID() const { return id_; }
    SNLID getSNLID() const;
    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    Type getType() const { return type_; }
    bool isStandard() const { return type_ == Type::Standard; }
    bool isInDB0() const { return type_ == Type::InDB0; }
    bool isPrimitives() const { return type_ == Type::Primitives; }

    friend bool operator< (const SNLLibrary &ll, const SNLLibrary &rl) {
      return ll.getSNLID() < rl.getSNLID();
    }
  private:
    static void preCreate(SNLDB* db, const Type& type, const SNLName& name);
    static void preCreate(SNLLibrary* parent, const Type& type, const SNLName& name);
    void destroyFromParent();
    void postCreate();
    void commonPreDestroy();
    void preDestroy() override;

    SNLLibrary(SNLDB* db, const Type& type, const SNLName& name);
    SNLLibrary(SNLLibrary* parent, const Type& type, const SNLName& name);

    void addLibrary(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);
    void addDesign(SNLDesign* design);
    void removeDesign(SNLDesign* design);

    using SNLLibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    using SNLLibraryDesignsHook =
      boost::intrusive::member_hook<SNLDesign, boost::intrusive::set_member_hook<>, &SNLDesign::libraryDesignsHook_>;
    using SNLLibraryDesigns = boost::intrusive::set<SNLDesign, SNLLibraryDesignsHook>;
    using SNLDesignNameIDMap = std::map<SNLName, SNLID::DesignID>;

    SNLID::LibraryID                    id_;
    SNLName                             name_             {};
    Type                                type_             { Type::Standard };
    void*                               parent_           { nullptr };
    bool                                isRootLibrary_    { false };
    boost::intrusive::set_member_hook<> librariesHook_    {};
    using SNLLibraryLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::librariesHook_>;
    using SNLLibraryLibraries = boost::intrusive::set<SNLLibrary, SNLLibraryLibrariesHook>;
    SNLLibraryLibraries                 libraries_        {}; 
    SNLLibraryNameIDMap                 libraryNameIDMap_ {};
    SNLLibraryDesigns                   designs_          {};
    SNLDesignNameIDMap                  designNameIDMap_  {};
};

}

#endif /* __SNL_LIBRARY_H_ */ 
