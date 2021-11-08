#ifndef __SNL_UNIVERSE_H_
#define __SNL_UNIVERSE_H_

#include "SNLDB.h"

namespace SNL {

/**
  * \brief SNLUniverse is a singleton class holding all SNL managed objects.
  *
  * Several SNLDB can live and share inside SNLUniverse.
  */

class SNLUniverse final: public SNLObject {
  public:
    friend class SNLDB;
    using super = SNLObject;
    SNLUniverse(const SNLUniverse&) = delete;

    ///\return a created singleton SNLUniverse or an error if it exists already
    static SNLUniverse* create();
    ///\return the singleron SNLUniverse or null if it does not exist.
    static SNLUniverse* get();

    SNLCollection<SNLDB> getDBs();

    ///\return the SNLDB with SNLID::DBID:id or null if it does not exist
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
