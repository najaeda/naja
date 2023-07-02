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

#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLSharedPath;

/**
 * \brief SNLPath des
*/
class SNLPath {
  public:
    friend class SNLOccurrence;

    SNLPath() = default;
    SNLPath(const SNLPath&);
    SNLPath(SNLSharedPath* sharedPath);
    SNLPath(SNLInstance* instance);
    SNLPath(const SNLPath& headPath, SNLInstance* tailInstance);
    SNLPath(SNLInstance* headInstance, const SNLPath& tailPath);

    SNLInstance* getHeadInstance() const;
    SNLPath getTailPath() const;
    SNLPath getHeadPath() const;
    SNLInstance* getTailInstance() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;

    bool empty() const;

    SNLPath& operator=(const SNLPath& path) = default;
    bool operator==(const SNLPath& path) const;
    bool operator!=(const SNLPath& path) const;
    bool operator<(const SNLPath& path) const;

  private:
    SNLSharedPath* getSharedPath() const { return sharedPath_; }
    SNLSharedPath*  sharedPath_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PATH_H_