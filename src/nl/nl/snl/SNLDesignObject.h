// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_DESIGN_OBJECT_H_
#define __SNL_DESIGN_OBJECT_H_

#include "NLObject.h"
#include "NLID.h"
#include "NLName.h"
#include "SNLAttributes.h"

namespace naja { namespace NL {

class NLDB;
class NLLibrary;
class NLDesign;

class SNLDesignObject: public NLObject {
  public:
    using super = NLObject;
    SNLDesignObject(const SNLDesignObject&) = delete;
    SNLDesignObject(const SNLDesignObject&&) = delete;

    struct PointerLess {
      bool operator()(const SNLDesignObject* lo, const SNLDesignObject* ro) const {
        return *lo < *ro;
      }
    };

    /// \return the owner SNLDesign of this SNLDesignObject.
    virtual SNLDesign* getDesign() const = 0;
    /// \return the unique NLID of this SNLDesignObject.
    virtual NLID getNLID() const = 0;
    NLID getNLID(const NLID::Type& type,
        NLID::DesignObjectID id,
        NLID::DesignObjectID instanceID,
        NLID::Bit bit) const;
    /// \return the owner NLLibrary of this SNLDesignObject.
    NLLibrary* getLibrary() const;

    NajaCollection<SNLAttribute> getAttributes() const;

    /// \return the owner NLDB of this SNLDesignObject.
    NLDB* getDB() const;
    /// \return true if this SNLDesignObject is unnamed, false if not.
    virtual bool isUnnamed() const = 0;
    /**
     * \brief set the SNLName of this SNLDesignObject
     * \warning this method will throw an exception if used on SNLBusTermBit, SNLBusNetBit or SNLInstTermBit
     * or if the name is already used in the design.
    */
    virtual void setName(const NLName& name) = 0;
    
    /**
     * \brief Less Compare two SNLDesignObjets by their NLID.
     * \param rhs Right Hand Side SNLDesignObject.
     */
    bool operator<(const SNLDesignObject &rhs) const {
      return getNLID() < rhs.getNLID();
    }

    /**
     * \brief Equality Compare two SNLDesignObjets by their NLID.
     * \param rhs Right Hand Side SNLDesignObject.
     */
    bool operator==(const SNLDesignObject &rhs) const {
      return getNLID() == rhs.getNLID();
    }
  protected:
    SNLDesignObject() = default;

    void postCreate();
    void preDestroy() override;
};

}} // namespace NL // namespace naja

#endif // __SNL_DESIGN_OBJECT_H_