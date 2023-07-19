// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

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
 * - storing Occurrence properties
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
    SNLID                               key_                      ;
    SNLSharedPath*                      headSharedPath_           {nullptr};
    SNLInstance*                        tailInstance_             {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_SHARED_PATH_H_
