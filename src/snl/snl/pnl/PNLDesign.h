// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_H_
#define __PNL_DESIGN_H_

#include "SNLObject.h"
#include "SNLID.h"
#include "PNLPoint.h"

namespace naja { namespace SNL {
  class SNLDB;
  class SNLLibrary;
}}

namespace naja { namespace PNL {

class PNLDesign final: public naja::SNL::SNLObject {

  public:
    friend class naja::SNL::SNLLibrary;
    using super = naja::SNL::SNLObject;
    static PNLDesign* create(naja::SNL::SNLLibrary* library);

    ///\return owning SNLDB
    naja::SNL::SNLDB* getDB() const;
    /// \return owning SNLLibrary.
    naja::SNL::SNLLibrary* getLibrary() const { return library_; }

    naja::SNL::SNLID::DesignID getID() const { return id_; }
    naja::SNL::SNLID getSNLID() const;

    friend bool operator< (const PNLDesign& ld, const PNLDesign& rd) {
      return ld.getSNLID() < rd.getSNLID();
    }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(const PNLDesign* other, std::string& reason) const;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;
  
  private:
    PNLDesign(naja::SNL::SNLLibrary* library);
    static void preCreate(const naja::SNL::SNLLibrary* library);
    void postCreateAndSetID();
    void postCreate();
    void preDestroy() override;

    naja::SNL::SNLID::DesignID                id_;
    naja::SNL::SNLLibrary*                    library_            {};
    naja::PNL::PNLPoint                       origin_             {0, 0};
    boost::intrusive::set_member_hook<>       libraryDesignsHook_ {};
};

}} // namespace PNL // namespace naja

#endif // __PNL_DESIGN_H_