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

#ifndef __NAJA_PRIVATE_PROPERTY_H_
#define __NAJA_PRIVATE_PROPERTY_H_

#include "NajaProperty.h"

namespace naja {

class NajaPrivateProperty: public NajaProperty {
  public:
    using super = NajaProperty;

    NajaObject* getOwner() const { return owner_; }
  protected:
    NajaPrivateProperty() = default;
    static void preCreate(const NajaObject* object, const std::string& name);
    void postCreate(NajaObject* owner);
    void preDestroy() override;
    void onCapturedBy(NajaObject* object) override;
    void onReleasedBy(const NajaObject* object) override;
  private:
    NajaObject* owner_  {nullptr};
};

} // namespace naja

#endif /* __NAJA_PRIVATE_PROPERTY_H_ */