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

    SNLLibrary() = delete;

    static SNLLibrary* create(SNLDB* db, const SNLName& name = SNLName());
    static SNLLibrary* create(SNLLibrary* parent, const SNLName& name = SNLName());

    bool isRootLibrary() const { return isRootLibrary_; }

    SNLDB* getDB() const;

    SNLCollection<SNLLibrary> getLibraries();
    SNLLibrary* getParentLibrary() const;
    SNLLibrary* getLibrary(SNLID::LibraryID id);
    SNLLibrary* getLibrary(const SNLName& name);

    SNLCollection<SNLDesign> getDesigns();
    SNLDesign* getDesign(SNLID::DesignID id);
    SNLDesign* getDesign(const SNLName& name);

    SNLID::LibraryID getID() const { return id_; }
    SNLID getSNLID() const;
    SNLName getName() const { return name_; }
    bool isAnonymous() const { return name_.empty(); }
    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    friend bool operator< (const SNLLibrary &ll, const SNLLibrary &rl) {
      return ll.getSNLID() < rl.getSNLID();
    }
  private:
    static void preCreate(const SNLDB* db, const SNLName& name);
    static void preCreate(const SNLLibrary* parent, const SNLName& name);
    void destroyFromDB();
    void postCreate();
    void preDestroy() override;

    SNLLibrary(SNLDB* db, const SNLName& name);
    SNLLibrary(SNLLibrary* parent, const SNLName& name);

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
    SNLName                             name_;
    void*                               parent_;
    bool                                isRootLibrary_;
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
