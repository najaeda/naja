// SPDX-FileCopyrightText: 2025 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_H_
#define __PNL_DESIGN_H_

#include "NLDesign.h"
#include "NLName.h"
#include "NLObject.h"
#include "NLID.h"

namespace naja { namespace NL {

class NLDB;
class NLLibrary;

class PNLDesign final: public NLObject {
  public:
    friend class NLLibrary;
    using super = NLObject;
    //using PNLDesignSlaveInstancesHook =
    //  boost::intrusive::member_hook<PNLInstance, boost::intrusive::set_member_hook<>, &PNLInstance::designSlaveInstancesHook_>;
    //using PNLDesignSlaveInstances = boost::intrusive::set<PNLInstance, PNLDesignSlaveInstancesHook>;
    
    static PNLDesign* create(NLLibrary* library, const NLName& name=NLName());

 
    NLID::DesignID getID() const { return id_; }

    ///\return owning NLDB
    NLDB* getDB() const;
    /// \return owning NLLibrary.
    NLLibrary* getLibrary() const { return library_; }

    /// \return NLName of this PNLDesign. 
    NLName getName() const { return name_; }
    /// \return true if this PNLDesign is anonymous.
    bool isAnonymous() const { return name_.empty(); }

    const char* getTypeName() const override;
    std::string getString() const override;
    std::string getDescription() const override;
    bool deepCompare(
      const PNLDesign* other,
      std::string& reason,
      NLDesign::CompareType type=NLDesign::CompareType::Complete) const;
    void debugDump(size_t indent, bool recursive=true, std::ostream& stream=std::cerr) const override;

   
  private:
    void destroyFromLibrary();

    NLID::DesignID                      id_;
    NLName                              name_               {};
    NLLibrary*                          library_            {};
    boost::intrusive::set_member_hook<> libraryDesignsHook_ {};
};

}} // namespace NL // namespace naja

#endif // __PNL_DESIGN_H_
