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
    //returns this SNLSharedPath key
    SNLID getKey() const { return key_; }
    size_t size() const;

    struct KeyComp {
      bool operator()(const SNLSharedPath& lp, const SNLSharedPath& rp) const {
        return lp.getKey() < rp.getKey();
      }
      bool operator()(const SNLID& key, const SNLSharedPath& rsp) const {
        return key < rsp.getKey();
      }
      bool operator()(const SNLSharedPath& lsp, const SNLID& key) const {
        return lsp.getKey() < key;
      }
    };

    std::string getString(char separator='/');

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
