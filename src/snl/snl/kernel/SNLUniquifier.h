// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef __SNL_UNIQUIFIER_H_
#define __SNL_UNIQUIFIER_H_

#include "NajaCollection.h"
#include "SNLID.h"

namespace naja { namespace SNL {

class SNLPath;
class SNLInstance;

class SNLUniquifier {
  public:
    SNLUniquifier(const std::vector<SNLID::DesignObjectID>& path, size_t id, bool uniquifyTail = false)
      :path_(path), id_(std::to_string(id)), uniquifyTail_(uniquifyTail) {}
    SNLUniquifier(const std::vector<SNLID::DesignObjectID>& path, std::string id, bool uniquifyTail = false)
      :path_(path), id_(id), uniquifyTail_(uniquifyTail) {}
    SNLUniquifier(const SNLPath& path, bool uniquifyTail = true);
    void process();
    SNLInstance* replaceWithClone(SNLInstance* inst);
    std::vector<SNLInstance*>& getPathUniq() { return pathUniq_; }
    NajaCollection<SNLInstance*> getPathUniqCollection() { return new NajaSTLCollection(&pathUniq_); }
    std::string getFullPath() const;
    std::string getString() const { return getFullPath(); }

    // Comparators
    bool operator==(const SNLUniquifier& other) const {
      return path_ == other.path_ && id_ == other.id_ && uniquifyTail_ == other.uniquifyTail_;
    }
    bool operator!=(const SNLUniquifier& other) const { return not operator==(other); }
    bool operator<(const SNLUniquifier& other) const {
      return path_ < other.path_ || (path_ == other.path_ && id_ < other.id_) ||
             (path_ == other.path_ && id_ == other.id_ && uniquifyTail_ < other.uniquifyTail_);
    }
    bool operator>(const SNLUniquifier& other) const {
      return path_ > other.path_ || (path_ == other.path_ && id_ > other.id_) ||
             (path_ == other.path_ && id_ == other.id_ && uniquifyTail_ > other.uniquifyTail_);
    }
    bool operator<=(const SNLUniquifier& other) const { return not operator>(other); }
    bool operator>=(const SNLUniquifier& other) const { return not operator<(other); }

  private:
    std::vector<SNLID::DesignObjectID> path_;
    std::vector<SNLInstance*> pathUniq_;
    std::string id_;
    bool uniquifyTail_ = false;
};

}} // namespace SNL // namespace naja

#endif  // __SNL_UNIQUIFIER_H_