#ifndef __SNL_UNIVERSE_H_
#define __SNL_UNIVERSE_H_

#include "SNLDB.h"

namespace SNL {

class SNLUniverse final: public SNLObject {
  public:
    friend class SNLDB;
    using super = SNLObject;
    SNLUniverse(const SNLUniverse&) = delete;

    static SNLUniverse* create();
    static SNLUniverse* get();

    SNLCollection<SNLDB> getDBs();
    SNLDB* getDB(SNLID::DBID id);

    constexpr const char* getTypeName() const override;
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
    SNLUniverseDBs      dbs_;
};

}

#endif /* __SNL_UNIVERSE_H_ */
