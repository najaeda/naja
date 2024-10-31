// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_OBJECT_H_
#define __SNL_DESIGN_OBJECT_H_

#include "SNLObject.h"
#include "SNLID.h"
#include "SNLName.h"

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

    /// \return the owner SNLDesign of this SNLDesignObject.
    virtual SNLDesign* getDesign() const = 0;
    /// \return the unique SNLID of this SNLDesignObject.
    virtual SNLID getSNLID() const = 0;
    SNLID getSNLID(const SNLID::Type& type,
        SNLID::DesignObjectID id,
        SNLID::DesignObjectID instanceID,
        SNLID::Bit bit) const;
    /// \return the owner SNLLibrary of this SNLDesignObject.
    SNLLibrary* getLibrary() const;
    /// \return the owner SNLDB of this SNLDesignObject.
    SNLDB* getDB() const;
    /// \return true if this SNLDesignObject is anonymous, false if not.
    virtual bool isAnonymous() const = 0;
    /**
     * \brief set the SNLName of this SNLDesignObject
     * \warning this method will throw an exception if used on SNLBusTermBit, SNLBusNetBit or SNLInstTermBit
     * or if the name is already used in the design.
    */
    virtual void setName(const SNLName& name) = 0;
    
    /**
     * \brief Less Compare two SNLDesignObjets by their SNLID.
     * \param rhs Right Hand Side SNLDesignObject.
     */
    bool operator<(const SNLDesignObject &rhs) const {
      return getSNLID() < rhs.getSNLID();
    }

    /**
     * \brief Equality Compare two SNLDesignObjets by their SNLID.
     * \param rhs Right Hand Side SNLDesignObject.
     */
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