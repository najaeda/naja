#ifndef __SNL_DB_H_
#define __SNL_DB_H_

#include "SNLObject.h"
#include "SNLLibrary.h"

namespace SNL {

class SNLDB final: public SNLObject {
  public:
    friend class SNLLibrary;

    using SNLDBLibrariesHook =
      boost::intrusive::member_hook<SNLLibrary, boost::intrusive::set_member_hook<>, &SNLLibrary::librariesHook_>;
    using SNLDBLibraries = boost::intrusive::set<SNLLibrary, SNLDBLibrariesHook>;

    using super = SNLObject;

    static SNLDB* create();

    SNLLibrary* getLibrary(const SNLName& name);

    constexpr const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
  private:
    static void preCreate();
    void postCreate();
    void preDestroy() override;

    void addLibrary(SNLLibrary* library);
    void removeLibrary(SNLLibrary* library);

    SNLDB() = default;

    SNLDBLibraries  libraries_  {};
};

}

#endif /* __SNL_DB_H_ */ 
