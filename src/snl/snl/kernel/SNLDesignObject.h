// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_OBJECT_H_
#define __SNL_DESIGN_OBJECT_H_

#include "SNLObject.h"
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLDB;
class SNLLibrary;
class SNLDesign;

class SNLDesignObject: public SNLObject {
  public:
    using super = SNLObject;
    SNLDesignObject(const SNLDesignObject&) = delete;
    SNLDesignObject(const SNLDesignObject&&) = delete;

    struct PointerLess {
      bool operator()(const SNLDesignObject* lo, const SNLDesignObject* ro) const {
        return *lo < *ro;
      }
    };

    ///\return the owner SNLDesign of this SNLDesignObject
    virtual SNLDesign* getDesign() const = 0;
    virtual SNLID getSNLID() const = 0;
    SNLID getSNLID(const SNLID::Type& type,
        SNLID::DesignObjectID id,
        SNLID::DesignObjectID instanceID,
        SNLID::Bit bit) const;
    ///\return the owner SNLLibrary of this SNLDesignObject
    SNLLibrary* getLibrary() const;
    ///\return the owner SNLDB of this SNLDesignObject
    SNLDB* getDB() const;
    ///\return true if this SNLDesignObject is anonymous, false if not.
    virtual bool isAnonymous() const = 0;

    bool operator<(const SNLDesignObject &rhs) const {
      return getSNLID() < rhs.getSNLID();
    }
    bool operator==(const SNLDesignObject &rhs) const {
      return getSNLID() == rhs.getSNLID();
    }
  protected:
    SNLDesignObject() = default;

    void postCreate();
    void preDestroy() override;
};

}} // namespace SNL // namespace naja

#endif // __SNL_DESIGN_OBJECT_H_