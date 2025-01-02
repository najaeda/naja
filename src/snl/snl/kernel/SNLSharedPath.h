// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_SHARED_PATH_H_
#define __SNL_SHARED_PATH_H_

#include <boost/intrusive/set.hpp>
#include "SNLID.h"
#include <vector>

namespace naja { namespace SNL {

class SNLInstance;
class SNLDesign;

/**
 * SNLSharedPath is a class non visible from users. Visible API is SNLPath.
 * SNLSharedPath class allows:
 * - caching SNLInstance path through a tree like structure, sharing common headPath
 * - storing Occurrence properties (in the future)
 * SNLSharedPath is a data structure composed of [SNLSharedPath, SNLInstance*] or [HeadSharedPath, tailInstance].
 * 
 * SNLSharedPath holds a key used for:
 * - comparing two Paths. 
 * - Inserting a new SNLSharedPath in an instance
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
    //returns this SNLSharedPath key
    size_t size() const;


    std::string getString(char separator='/');

    std::vector<SNLID::DesignObjectID> getPathIDs() const;

  private:
    SNLSharedPath(SNLInstance* tailInstance, SNLSharedPath* headSharedPath=nullptr);
    ~SNLSharedPath() = default;

    void commonDestroy();
    void destroy();
    void destroyFromInstance();

    boost::intrusive::set_member_hook<> instanceSharedPathsHook_  {};
    SNLSharedPath*                      headSharedPath_           {nullptr};
    SNLInstance*                        tailInstance_             {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_SHARED_PATH_H_
