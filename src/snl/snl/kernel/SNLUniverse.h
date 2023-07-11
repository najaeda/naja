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

#ifndef __SNL_UNIVERSE_H_
#define __SNL_UNIVERSE_H_

#include "SNLDB.h"

namespace naja { namespace SNL {

class SNLBusTermBit;

/**
 * \brief SNLUniverse is a singleton class holding all SNL managed objects.
 *
 * Several SNLDB can live and share inside SNLUniverse.
 */
class SNLUniverse final: public SNLObject {
  public:
    friend class SNLDB;
    friend class SNLDB0;
    using super = SNLObject;
    SNLUniverse(const SNLUniverse&) = delete;

    ///\return a created singleton SNLUniverse or an error if it exists already
    static SNLUniverse* create();
    ///\return the singleron SNLUniverse or null if it does not exist.
    static SNLUniverse* get();

    ///\return the collection of all SNLDB
    NajaCollection<SNLDB*> getDBs() const;

    ///\return the collection of user SNLDB
    NajaCollection<SNLDB*> getUserDBs() const;

    ///\return the SNLDB with SNLID::DBID id or null if it does not exist
    SNLDB* getDB(SNLID::DBID id) const;

    ///\return the SNLLibrary with SNLID::DBID dbID and SNLID::LibraryID or null if it does not exist
    SNLLibrary* getLibrary(SNLID::DBID dbID, SNLID::LibraryID libraryID) const;

    ///\return the SNLDesign with SNLID::DesignReference reference or null if it does not exist
    SNLDesign* getDesign(const SNLID::DesignReference& reference) const;

    ///\return the SNLDesign named name or null if it does not exist
    ///DBs are browsed in their index ordering
    SNLDesign* getDesign(const SNLName& name) const;

    ///\return the SNLTerm with SNLID::DesignObjectReference reference or null if it does not exist
    SNLTerm* getTerm(const SNLID::DesignObjectReference& reference) const;

    ///\return the SNLNet with SNLID::DesignObjectReference reference or null if it does not exist
    SNLNet* getNet(const SNLID::DesignObjectReference& reference) const;

    ///\return the SNLBitNet with SNLID::BitNetReference reference or null if it does not exist
    SNLBitNet* getBitNet(const SNLID::BitNetReference& reference) const;

    ///\return the SNLInstance with SNLID::DesignObjectReference reference or null if it does not exist
    SNLInstance* getInstance(const SNLID::DesignObjectReference& reference) const;

    ///\return the SNLInstTerm with SNLID id or null if it does not exist
    SNLInstTerm* getInstTerm(const SNLID& id) const;

    ///\return the SNLBusTermBit with SNLID id or null if it does not exist
    SNLBusTermBit* getBusTermBit(const SNLID& id) const;

    ///\return the SNLBusNetBit with SNLID id or null if it does not exist
    SNLBusNetBit* getBusNetBit(const SNLID& id) const;

    ///\return the SNLObject with SNLID id or null if it does not exist
    SNLObject* getObject(const SNLID& id);
    
    SNLDB* getTopDB() const;
    SNLDesign* getTopDesign() const;
    void setTopDB(SNLDB* db);
    void setTopDesign(SNLDesign* design);

    static bool isDB0(const SNLDB* db);
    static SNLDB* getDB0();

    void mergeAssigns();
    
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLUniverse() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;

    void addDBAndSetID(SNLDB* db);
    void addDB(SNLDB* db);
    void removeDB(SNLDB* db);
    
    using SNLUniverseDBsHook =
      boost::intrusive::member_hook<SNLDB, boost::intrusive::set_member_hook<>, &SNLDB::universeDBsHook_>;
    using SNLUniverseDBs = boost::intrusive::set<SNLDB, SNLUniverseDBsHook>;

    static SNLUniverse* universe_;
    SNLUniverseDBs      dbs_          {};
    SNLDB*              db0_          {nullptr};
    SNLDB*              topDB_        {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_UNIVERSE_H_
