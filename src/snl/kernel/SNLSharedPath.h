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

namespace naja { namespace SNL {

class SNLInstance;
class SNLDesign;

/**
 * SNLSharedPath is a class non visible from users. Visible API is SNLPath.
 * SNLSharedPath class allows:
 * - caching SNLInstance path through a tree like structure, sharing common headPath
 * - storing Occurrence properties
 */
class SNLSharedPath {
  public:
    friend class SNLPath;
    SNLSharedPath() = delete;
    SNLSharedPath(const SNLSharedPath&) = delete;

    SNLInstance* getHeadInstance() const { return headInstance_; }
    SNLSharedPath* getTailSharedPath() const { return tailSharedPath_; }
    SNLSharedPath* getHeadSharedPath() const;
    SNLInstance* getTailInstance() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;

  private:
    SNLSharedPath(SNLInstance* headInstance, SNLSharedPath* tailSharedPath=nullptr); 

    SNLInstance*                        headInstance_             {nullptr};
    SNLSharedPath*                      tailSharedPath_           {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_SHARED_PATH_H_