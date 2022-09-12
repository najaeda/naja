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

#ifndef __NAJA_PROPERTY_H_
#define __NAJA_PROPERTY_H_

#include <string>

namespace naja {

class NajaObject;

class NajaProperty {
  public:
    friend class NajaObject;
    virtual std::string getName() const =0;
    virtual std::string getString() const =0;

    virtual bool isDumpable() const {
      return false;
    }
    void destroy();
  protected:
    NajaProperty() = default;
    virtual ~NajaProperty() = default;
    static void preCreate() {}
    void postCreate() {}
    virtual void preDestroy();

    virtual void onCapturedBy(NajaObject* object) =0;
    virtual void onReleasedBy(const NajaObject* object) =0;
};

} // namespace naja

#endif /* __NAJA_PROPERTY_H_ */
