// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifndef DNL_H
#define DNL_H

#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stack>
#include <vector>
#include "SNLUniverse.h"
#include "tbb/scalable_allocator.h"
#include <tbb/task_arena.h>
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLUniverse.h"
#include "tbb/parallel_for.h"

using namespace naja::SNL;

namespace naja {
namespace SNL {
class SNLBitNet;
}
}  // namespace naja

namespace naja {
namespace DNL {

typedef size_t DNLID;
constexpr DNLID DNLID_MAX = ((DNLID)-1);

template <class DNLInstance, class DNLTerminal>
class DNL;
class DNLTerminalFull;
class DNLInstanceFull;

typedef DNL<DNLInstanceFull, DNLTerminalFull> DNLFull;

// DNL<DNLInstanceFull, DNLTerminalFull>* create();
DNL<DNLInstanceFull, DNLTerminalFull>* get();
bool isCreated();
void destroy();

class DNLInstanceFull {
 public:
  DNLInstanceFull() = default;
  DNLInstanceFull(SNLInstance* instance, DNLID id, DNLID parent);
  void display() const;
  DNLID getID() const;
  std::string getFullPath() const {
    std::vector<SNLInstance*> path;
    DNLID instID = getID();
    DNLInstanceFull inst = *this;
    SNLInstance* snlInst = inst.getSNLInstance();
    if (snlInst != nullptr) {
      path.push_back(snlInst);
    }
    
    instID = inst.getParentID();
    while (instID != DNLID_MAX) {
      inst = inst.getParentInstance();
      snlInst = inst.getSNLInstance();
      if (snlInst != nullptr) {
        path.push_back(snlInst);
      }
      instID = inst.getParentID();
    }
    std::string fullPath;
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      fullPath += (*it)->getName().getString() + "/";
    }
    return fullPath;
  };
  DNLID getParentID() const;
  const DNLInstanceFull& getParentInstance() const;
  SNLInstance* getSNLInstance() const;
  const SNLDesign* getSNLModel() const;
  void setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes);
  void setChildrenIndexes(const std::pair<DNLID, DNLID>& childrenIndexes);
  const DNLInstanceFull& getChildInstance(const SNLInstance* snlInst) const;
  const DNLTerminalFull& getTerminal(const SNLInstTerm* snlTerm) const;
  const DNLTerminalFull& getTerminalFromBitTerm(
      const SNLBitTerm* snlTerm) const;
  const std::pair<DNLID, DNLID>& getTermIndexes() const {
    return termsIndexes_;
  }
  bool isNull() const { return id_ == (DNLID)DNLID_MAX; }
  bool isTop() const { return parent_ == (DNLID)DNLID_MAX; }
  const std::pair<DNLID, DNLID>& getChildren() const {
    return childrenIndexes_;
  }
  bool isLeaf() const {
    return childrenIndexes_.first == childrenIndexes_.second;
  }

 private:
  std::pair<DNLID, DNLID> childrenIndexes_;
  SNLInstance* instance_{nullptr};
  DNLID id_ = DNLID_MAX;
  DNLID parent_ = DNLID_MAX;
  std::pair<DNLID, DNLID> termsIndexes_;
};

class DNLTerminalFull {
 public:
  DNLTerminalFull() = default;
  DNLTerminalFull(DNLID id);
  DNLTerminalFull(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id);
  DNLTerminalFull(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id);
  DNLID getID() const;
  SNLInstTerm* getSnlTerm() const;
  SNLBitTerm* getSnlBitTerm() const;
  const DNLInstanceFull& getDNLInstance() const;
  bool isNull() const { return id_ == (DNLID)DNLID_MAX; }
  void setIsoID(DNLID isoID);
  DNLID getIsoID() const;
  bool isTopPort() const { return terminal_ == nullptr; }

 private:
  DNLID DNLInstID_ = DNLID_MAX;
  SNLInstTerm* terminal_{nullptr};
  SNLBitTerm* bitTerminal_{nullptr};
  DNLID id_ = DNLID_MAX;
};

class DNLIso {
 public:
  DNLIso(DNLID id = DNLID_MAX);
  void setId(DNLID id) { id_ = id; }
  virtual void addDriver(DNLID driver);
  virtual void addReader(DNLID reader);
  virtual void addHierTerm(DNLID hier) {}
  virtual void addNet(SNLBitNet* net) {}
  virtual void display(std::ostream& stream = std::cout) const;
  virtual DNLID getIsoID() const { return id_; }
  virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getDrivers()
      const {
    return drivers_;
  }
  virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getReaders()
      const {
    return readers_;
  }
  virtual ~DNLIso() {}

 private:
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> drivers_;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> readers_;
  DNLID id_ = DNLID_MAX;
};

class DNLComplexIso : public DNLIso {
 public:
  DNLComplexIso(DNLID id = DNLID_MAX) : DNLIso(id) {}
  void addNet(SNLBitNet* net) { _nets.insert(net); }
  const std::set<SNLBitNet*>& getNets() const { return _nets; }
  void addHierTerm(DNLID hier) { _hierTerms.push_back(hier); }
  const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getHierTerms() { return _hierTerms; }
 private:
  std::set<SNLBitNet*> _nets;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> _hierTerms;
};

class DNLIsoDB {
 public:
  DNLIsoDB();
  void display() const;
  DNLIso& addIso();
  DNLIso& getIsoFromIsoID(DNLID isoID) { return isos_[isoID]; }
  const DNLIso& getIsoFromIsoIDconst(DNLID isoID) const { if (isoID == DNLID_MAX) {return isos_.back();} return isos_[isoID]; }
  DNLID getNumIsos() const { return isos_.size(); }
  std::vector<DNLID> getFullIso(DNLID);

 private:
  std::vector<DNLIso, tbb::scalable_allocator<DNLIso>> isos_;
};

template <class DNLInstance, class DNLTerminal>
class DNLIsoDBBuilder {
 public:
  DNLIsoDBBuilder(DNLIsoDB& db, const DNL<DNLInstance, DNLTerminal>& dnl);
  void treatDriver(const DNLTerminal& term, DNLIso& DNLIso);
  void process();

 private:
  DNLIso& addIsoToDB() { return db_.addIso(); }
  DNLIsoDB& db_;
  DNL<DNLInstance, DNLTerminal> dnl_;
  std::vector<bool> visited_;
};

template <class DNLInstance, class DNLTerminal>
class DNL {
 public:
  DNL(const SNLDesign* top);
  void process();
  void display() const;
  void getCustomIso(DNLID dnlIsoId, DNLIso& DNLIso) const;
  const std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>>&
  getDNLInstances() const {
    return DNLInstances_;
  }
  const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getLeaves() {
    return leaves_;
  }
  const std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>>&
  getDNLTerms() const {
    return DNLTerms_;
  }
  const DNLTerminal& getDNLTerminalFromID(DNLID id) const {
    return DNLTerms_[id];
  }
  DNLTerminal& getDNLTerminalFromID(DNLID id) { return DNLTerms_[id]; }
  const DNLInstance& getDNLInstanceFromID(DNLID id) const {
    return DNLInstances_[id];
  }
  DNLInstance& getNonConstDNLInstanceFromID(DNLID id) {
    return DNLInstances_[id];
  }
  const DNLTerminal& getDNLNullTerminal() const { return DNLTerms_.back(); }
  DNLID getNBterms() const { return DNLTerms_.size() - 1; }
  const DNLInstance& getDNLNullInstance() const { return DNLInstances_.back(); }
  const DNLIsoDB& getDNLIsoDB() const { return fidb_; }
  std::vector<DNLID> getLeavesUnder(DNLID parent) const;
  const DNLInstance& getTop() const { return DNLInstances_[0]; }
  bool isInstanceChild(DNLID parent, DNLID child) const;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> getLeaves() const {
    return leaves_;
  }
  void initTermId2isoId() {
    termId2isoId_ = std::vector<DNLID>(DNLTerms_.size(), DNLID_MAX);
  }
  void setIsoIdforTermId(DNLID isoId, DNLID termId) {
    assert(termId < termId2isoId_.size());
    termId2isoId_[termId] = isoId;
  }
  DNLID getIsoIdfromTermId(DNLID termId) const {
    if (termId < termId2isoId_.size()) {
      return termId2isoId_[termId];
    }
    return DNLID_MAX;
  }

 private:
  std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>> DNLInstances_;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> leaves_;
  const SNLDesign* top_;
  std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>> DNLTerms_;
  std::vector<DNLID> termId2isoId_;
  DNLIsoDB fidb_;
};

#include "DNL_impl.h"

}  // namespace DNL
}  // namespace naja
#endif // DNL_H
