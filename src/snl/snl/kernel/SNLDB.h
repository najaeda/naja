// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DB_H_
#define __SNL_DB_H_

#include "SNLObject.h"
#include "SNLLibrary.h"

namespace naja { namespace SNL {

class SNLUniverse;

class SNLDB final: public SNLObject {
  public:
    friend class SNLUniverse;
    friend class SNLLibrary;
    using super = SNLObject;

    using SNLDBLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::dbLibrariesHook_>;
    using SNLDBLibraries = boost::intrusive::set<SNLLibrary, SNLDBLibrariesHook>;

    SNLDB(const SNLDB&) = delete;

    static SNLDB* create(SNLUniverse* universe);
    static SNLDB* create(SNLUniverse* universe, SNLID::DBID id);

    SNLID::DBID getID() const { return id_; }
    SNLID getSNLID() const;

    ///\return the SNLLibrary in this SNLDB with SNLID::LibraryID:id 
    SNLLibrary* getLibrary(SNLID::LibraryID id) const;
    ///\return the SNLLibrary in this SNLDB with SNLName:name 
    SNLLibrary* getLibrary(const SNLName& name) const;

    ///\return the SNLDesign with SNLID::DBDesignReference reference or null if it does not exist
    SNLDesign* getDesign(const SNLID::DBDesignReference& designReference) const;

    ///\return the SNLDesign named name or null if it does not exist
    ///Libraries are browsed in their index ordering
    SNLDesign* getDesign(const SNLName& name) const;

    bool isTopDB() const;
    SNLDesign* getTopDesign() const;
    void setTopDesign(SNLDesign* design);

    ///\return the Libraries owned by this SNLDB
    NajaCollection<SNLLibrary*> getLibraries() const;
    ///\return the all the Libraries owned (directly or indirectly) by this SNLDB
    NajaCollection<SNLLibrary*> getGlobalLibraries() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    friend bool operator< (const SNLDB &ldb, const SNLDB &rdb) {
      return ldb.getSNLID() < rdb.getSNLID();
    }
  private:
    SNLDB() = default;
    SNLDB(SNLID::DBID id);
    static void preCreate(SNLUniverse* universe);
    static void preCreate(SNLUniverse* universe, SNLID::DBID id);
    void postCreateAndSetID(SNLUniverse* universe);
    void postCreate(SNLUniverse* universe);
    void commonPreDrestroy();
    void preDestroy() override;
    void destroyFromUniverse();

    void addLibrary(SNLLibrary* library);
    void addLibraryAndSetID(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);

    SNLID::DBID                         id_;
    boost::intrusive::set_member_hook<> universeDBsHook_          {};
    SNLDBLibraries                      libraries_                {};
    using LibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    LibraryNameIDMap                    libraryNameIDMap_         {};
    SNLDesign*                          topDesign_                {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DB_H_
