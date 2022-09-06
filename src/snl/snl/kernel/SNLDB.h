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

#ifndef __SNL_DB_H_
#define __SNL_DB_H_

#include "NajaObject.h"
#include "SNLLibrary.h"

namespace naja { namespace SNL {

class SNLUniverse;

class SNLDB final: public NajaObject {
  public:
    friend class SNLUniverse;
    friend class SNLLibrary;
    using super = NajaObject;

    using SNLDBLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::librariesHook_>;
    using SNLDBLibraries = boost::intrusive::set<SNLLibrary, SNLDBLibrariesHook>;

    SNLDB() = delete;
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

    bool isTopDB() const;
    SNLDesign* getTopDesign() const;
    void setTopDesign(SNLDesign* design);

    NajaCollection<SNLLibrary*> getLibraries() const;

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;

    friend bool operator< (const SNLDB &ldb, const SNLDB &rdb) {
      return ldb.getSNLID() < rdb.getSNLID();
    }
  private:
    SNLDB(SNLUniverse* universe);
    SNLDB(SNLUniverse* universe, SNLID::DBID id);
    static void preCreate(SNLUniverse* universe);
    static void preCreate(SNLUniverse* universe, SNLID::DBID id);
    void postCreateAndSetID();
    void postCreate();
    void commonPreDrestroy();
    void preDestroy() override;
    void destroyFromUniverse();

    void addLibrary(SNLLibrary* library);
    void addLibraryAndSetID(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);

    SNLUniverse*                        universe_;
    SNLID::DBID                         id_;
    boost::intrusive::set_member_hook<> universeDBsHook_          {};
    SNLDBLibraries                      libraries_                {};
    using LibraryNameIDMap = std::map<SNLName, SNLID::LibraryID>;
    LibraryNameIDMap                    libraryNameIDMap_         {};
    SNLDesign*                          topDesign_                {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_DB_H_
