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

    auto getDBs() const;

    ///\return the SNLDB with SNLID::DBID:id or null if it does not exist
    SNLDB* getDB(SNLID::DBID id);

    static bool isDB0(const SNLDB* db);
    
    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    SNLUniverse() = default;
    static void preCreate();
    void postCreate();
    void preDestroy() override;

    void addDB(SNLDB* db);
    void removeDB(SNLDB* db);
    
    using SNLUniverseDBsHook =
      boost::intrusive::member_hook<SNLDB, boost::intrusive::set_member_hook<>, &SNLDB::universeDBsHook_>;
    using SNLUniverseDBs = boost::intrusive::set<SNLDB, SNLUniverseDBsHook>;

    static SNLUniverse* universe_;
    SNLUniverseDBs      dbs_          {};
    SNLDB*              db0_          {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_UNIVERSE_H_