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


#ifndef __SNL_OCCURRENCE_H_
#define __SNL_OCCURRENCE_H_

#include <compare>

namespace naja { namespace SNL {

class SNLPath;
class SNLSharedPath;
class SNLDesignObject;

class SNLOccurrence {
  public:
    SNLOccurrence()=default;
    //needs to be revised in case of refcount on sharedpath
    SNLOccurrence(const SNLOccurrence&)=default;

    SNLOccurrence(SNLDesignObject* object);
    SNLOccurrence(const SNLPath& path, SNLDesignObject* object);

    SNLPath getPath() const;
    SNLDesignObject* getObject() const { return object_; }
    bool isValid() const { return object_ != nullptr; }

    bool operator==(const SNLOccurrence& occurrence) const;
    bool operator<(const SNLOccurrence& occurrence) const;
    auto operator<=>(const SNLOccurrence& rhs) const = default;

  protected:
    SNLOccurrence(const SNLPath& path);
  private:
    SNLSharedPath*      path_   {nullptr};
    SNLDesignObject*    object_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_OCCURRENCE_H_
