#ifndef __SNL_DB_H_
#define __SNL_DB_H_

#include "SNLObject.h"
#include "SNLLibrary.h"

#include "SNLCollection.h"

namespace SNL {

class SNLUniverse;

class SNLDB final: public SNLObject {
  public:
    friend class SNLUniverse;
    friend class SNLLibrary;
    using super = SNLObject;

    using SNLDBLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::librariesHook_>;
    using SNLDBLibraries = boost::intrusive::set<SNLLibrary, SNLDBLibrariesHook>;

    SNLDB() = delete;
    SNLDB(const SNLDB&) = delete;

    static SNLDB* create(SNLUniverse* universe);

    SNLID::DBID getID() const { return id_; }
    SNLID getSNLID() const;

    ///\return the SNLLibrary in this SNLDB with SNLID::LibraryID:id 
    SNLLibrary* getLibrary(SNLID::LibraryID id);
    ///\return the SNLLibrary in this SNLDB with SNLName:name 
    SNLLibrary* getLibrary(const SNLName& name);

    SNLCollection<SNLLibrary> getLibraries();

    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    friend bool operator< (const SNLDB &ldb, const SNLDB &rdb) {
      return ldb.getSNLID() < rdb.getSNLID();
    }
  private:
    SNLDB(SNLUniverse* universe);
    static void preCreate();
    void postCreate();
    void preDestroy() override;
    void destroyFromUniverse();

    void addLibrary(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);

    SNLUniverse*      universe_;
    SNLID::DBID       id_;
    boost::intrusive::set_member_hook<> universeDBsHook_  {};
    SNLID::LibraryID  nextLibraryID_                      {0};
    SNLDBLibraries    libraries_                          {};
    using LibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    LibraryNameIDMap  libraryNameIDMap_                   {};
};

}

#endif /* __SNL_DB_H_ */ 
