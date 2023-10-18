// Copyright The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

#include <vector>

#include "SNLID.h"

namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLSharedPath;

/**
 * \brief SNLPath: SNLInstance path through hierarchy API
*/
class SNLPath {
  public:
    friend class SNLOccurrence;

    SNLPath() = default;
    SNLPath(const SNLPath&) = default;
    SNLPath(SNLSharedPath* sharedPath);
    SNLPath(SNLInstance* instance);
    SNLPath(const SNLPath& headPath, SNLInstance* tailInstance);
    SNLPath(SNLInstance* headInstance, const SNLPath& tailPath);

    using PathStringDescriptor = std::vector<std::string>;
    SNLPath(const SNLDesign* top, const PathStringDescriptor& descriptor);
    using PathIDDescriptor = std::vector<SNLID::DesignObjectID>;
    SNLPath(const SNLDesign* top, const PathIDDescriptor& descriptor);

    SNLInstance* getHeadInstance() const;
    SNLPath getTailPath() const;
    SNLPath getHeadPath() const;
    SNLInstance* getTailInstance() const;
    SNLDesign* getDesign() const;
    SNLDesign* getModel() const;

    bool empty() const;
    size_t size() const;

    SNLPath& operator=(const SNLPath& path) = default;
    bool operator==(const SNLPath& rhs) const;
    bool operator!=(const SNLPath& rhs) const;
    bool operator<(const SNLPath& rhs) const;

    std::string getString(const char separator='/') const;

  private:
    static SNLSharedPath* createInstanceSharedPath(SNLInstance* instance);
    SNLSharedPath* getSharedPath() const { return sharedPath_; }
    SNLSharedPath*  sharedPath_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PATH_H_
