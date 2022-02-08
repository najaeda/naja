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

#ifndef __SNL_DESIGN_OBJECT_H_
#define __SNL_DESIGN_OBJECT_H_

#include "SNLObject.h"
#include "SNLID.h"

namespace SNL {

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

    friend bool operator<(const SNLDesignObject &ldo, const SNLDesignObject &rdo) {
      return ldo.getSNLID() < rdo.getSNLID();
    }
  protected:
    SNLDesignObject() = default;

    void postCreate();
    void preDestroy() override;
};

}

#endif /* __SNL_DESIGN_OBJECT_H_ */
