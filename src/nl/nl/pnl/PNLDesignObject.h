// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __PNL_DESIGN_OBJECT_H_
#define __PNL_DESIGN_OBJECT_H_

#include "NLObject.h"
#include "NLID.h"

namespace naja { namespace NL {

class NLDB;
class NLLibrary;

class PNLDesign;

class PNLDesignObject: public NL::NLObject {
  public:
    virtual PNLDesign* getDesign() const = 0;

    virtual naja::NL::NLID getNLID() const = 0;
    naja::NL::NLID getNLID(const naja::NL::NLID::Type& type,
        naja::NL::NLID::DesignObjectID id,
        naja::NL::NLID::DesignObjectID instanceID,
        naja::NL::NLID::Bit bit) const;

  bool operator<(const PNLDesignObject &rhs) const {
      return getNLID() < rhs.getNLID();
  }

  naja::NL::NLLibrary* getLibrary() const;
  
  naja::NL::NLDB* getDB() const;

  virtual bool isAnonymous() const = 0;

  protected:
    PNLDesignObject() = default;
    
    static void preCreate();
    void postCreate();
};

}} // namespace NL // namespace naja

#endif // __PNL_DESIGN_OBJECT_H_