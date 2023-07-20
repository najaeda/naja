// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_PATH_H_
#define __SNL_PATH_H_

namespace naja { namespace SNL {

class SNLDesign;
class SNLInstance;
class SNLSharedPath;

class SNLPath {
  public:
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

  private:
    SNLSharedPath*  sharedPath_ {nullptr};
};

}} // namespace SNL // namespace naja

#endif // __SNL_PATH_H_