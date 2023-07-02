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

#ifndef __SNL_SHARED_PATH_H_
#define __SNL_SHARED_PATH_H_

#include <boost/intrusive/set.hpp>
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLInstance;
class SNLDesign;

/**
 * SNLSharedPath is a class non visible from users. Visible API is SNLPath.
 * SNLSharedPath class allows:
 * - caching SNLInstance path through a tree like structure, sharing common headPath
 * - storing Occurrence properties (in the future)
 * SNLSharedPath is a data structure composed of [SNLSharedPath, SNLInstance*]
 */
class SNLSharedPath {
  public:
    friend class SNLInstance;
    friend class SNLPath;
    SNLSharedPath() = delete;
    SNLSharedPath(const SNLSharedPath&) = delete;

    SNLSharedPath* getHeadSharedPath() const { return headSharedPath_; }
    SNLInstance* getTailInstance() const { return tailInstance_; }
    SNLInstance* getHeadInstance() const;
    SNLSharedPath* getTailSharedPath() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;
    ///returns this SNLSharedPath key
    SNLID getSNLID() const { return key_; }

    friend bool operator<(const SNLSharedPath& lp, const SNLSharedPath& rp) {
      return lp.getSNLID() < rp.getSNLID();
    }

  private:
    SNLSharedPath(SNLInstance* tailInstance, SNLSharedPath* headSharedPath=nullptr);
    ~SNLSharedPath() = default;

    void commonDestroy();
    void destroy();
    void destroyFromInstance();

    boost::intrusive::set_member_hook<> instanceSharedPathsHook_  {};
    SNLID                               key_;
    SNLSharedPath*                      headSharedPath_           {nullptr};
    SNLInstance*                        tailInstance_             {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_SHARED_PATH_H_
