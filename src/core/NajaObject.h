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

#ifndef __NAJA_OBJECT_H_
#define __NAJA_OBJECT_H_

#include <map>
#include <string>
#include "NajaCollection.h"

namespace naja {

class NajaProperty;

class NajaObject {
  public:
    using Properties = std::map<std::string, NajaProperty*>;
    virtual ~NajaObject() = default;

    ///\return a string describing the object type
    virtual const char* getTypeName() const = 0;
    ///\return a simple string describing the object. Usually object name.
    virtual std::string getString() const = 0;
    ///\return a string extensively describing the object. Useful for debug.
    virtual std::string getDescription() const = 0;

    ///\return NajaProperty with std::string name or nullptr if it does not exist
    NajaProperty* getProperty(const std::string& name) const;
    ///\return the collection of SNLParameter of this SNLDesign
    NajaCollection<NajaProperty*> getProperties() const;
  protected:
    NajaObject() = default;

  private:
    Properties  properties_;
};

} // namespace naja

#endif // __NAJA_OBJECT_H_