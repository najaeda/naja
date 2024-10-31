// SPDX-FileCopyrightText: 2024 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef DNL_H
#define DNL_H

#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stack>
#include <vector>
#include <map>
#include "SNLUniverse.h"
#include "tbb/scalable_allocator.h"
#include "tbb/concurrent_vector.h"
#include <tbb/task_arena.h>
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLUniverse.h"
#include "tbb/parallel_for.h"
#include "tbb/enumerable_thread_specific.h"

using namespace naja::SNL;

namespace naja {
namespace SNL {
class SNLBitNet;
}
}  // namespace naja

namespace naja {

class OrderIDInitializer {
 public:
  OrderIDInitializer() {};
  void process();
};

namespace DNL {

typedef size_t DNLID;
constexpr DNLID DNLID_MAX = ((DNLID)-1);

template <class DNLInstance, class DNLTerminal>
class DNL;
class DNLTerminalFull;
class DNLInstanceFull;

typedef DNL<DNLInstanceFull, DNLTerminalFull> DNLFull;

struct visited {
    std::vector<bool> visited;
    std::vector<bool> toVisitAsInstTerm;
    std::vector<bool> toVisitAsBitTerm;
};

struct SNLBitTermCompare {
  bool operator()(const SNLBitTerm* lhs, const SNLBitTerm* rhs) const {
    return lhs->getOrderID() < rhs->getOrderID();
  }
};

struct SNLInstTermCompare {
  bool operator()(const SNLInstTerm* lhs, const SNLInstTerm* rhs) const {
    return lhs->getBitTerm()->getOrderID() < rhs->getBitTerm()->getOrderID();
    /*if (lhs->getBitTerm()->getID() != rhs->getBitTerm()->getID()) {
      return lhs->getBitTerm()->getID() < rhs->getBitTerm()->getID();
    }
    return lhs->getBitTerm()->getBit() > rhs->getBitTerm()->getBit();*/
  }
};

// DNL<DNLInstanceFull, DNLTerminalFull>* create();
DNL<DNLInstanceFull, DNLTerminalFull>* get();
bool isCreated();
void destroy();

class DNLInstanceFull {
 public:
  DNLInstanceFull() = default;
  /**
     * \brief Create a DNLInstanceFull.
     * \param instance the SNLInstsance.
     * \param id DNL ID of the DNLInstance.
     * \param parent DNL ID of the parent DNLInstance.
     * \return the created DNLInstanceFull.
     */
  DNLInstanceFull(SNLInstance* instance, DNLID id, DNLID parent);
  /** 
   * \brief display content of the DNLInstanceFull.
   */
  void display() const;  
  /**
   * \brief get the DNL ID of the DNLInstanceFull.
   * \return the DNL ID of the DNLInstanceFull.
   */
  DNLID getID() const;
  /**
   * \brief get the SNLInstance of the DNLInstanceFull.
   * \return the SNLInstance of the DNLInstanceFull.
   */
  std::string getFullPath() const;
  /**
   * \brief get the parent DNL ID of the DNLInstanceFull.
   * \return the parent DNL ID of the DNLInstanceFull.
   */
  DNLID getParentID() const;
  /**
   * \brief get the DNLInstanceFull of the parent DNLInstanceFull.
   * \return the DNLInstanceFull of the parent DNLInstanceFull.
   */
  const DNLInstanceFull& getParentInstance() const;
  /**
   * \brief get the SNLInstance of the DNLInstanceFull.
   * \return the SNLInstance of the DNLInstanceFull.
   */
  SNLInstance* getSNLInstance() const;
  /**
   * \brief get the SNLDesign of the DNLInstanceFull.
   * \return the SNLDesign of the DNLInstanceFull.
   */
  const SNLDesign* getSNLModel() const;
  /**
   * \brief set the DNL ID of instancde terms.
   * \param termsIndexes the DNL ID of instance terms.
   */
  void setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes);
  /**
   * \brief set the DNL ID of children instances.
   * \param childrenIndexes the DNL ID of children instances.
   */
  void setChildrenIndexes(const std::pair<DNLID, DNLID>& childrenIndexes);
  /**
   * \brief get the DNLInstanceFull of the child snlInstance.
   * \param snlInst the SNLInstance of the child DNLInstanceFull.
   * \return the DNLInstanceFull of the snlInst in current context.
   */
  const DNLInstanceFull& getChildInstance(const SNLInstance* snlInst) const;
  /**
   * \brief get the DNLTerminalFull by the SNLInstTerm.
   * \param snlTerm the SNLInstTerm of the DNLTerminalFull.
   * \return the DNLTerminalFull of the snlTerm in current context.
   */
  const DNLTerminalFull& getTerminal(const SNLInstTerm* snlTerm) const;
  /**
   * \brief get the DNLTerminalFull by the SNLBiterm.
   * \param snlTerm the SNLBitTerm of the DNLTerminalFull.
   * \return the DNLTerminalFull of the snlTerm in current context.
   */
  const DNLTerminalFull& getTerminalFromBitTerm(
      const SNLBitTerm* snlTerm) const;
  const std::pair<DNLID, DNLID>& getTermIndexes() const {
    return termsIndexes_;
  }
  /**
   * \brief check if the DNLInstanceFull is null.
   * \return true if the DNLInstanceFull is null.
   */
  bool isNull() const { return id_ == (DNLID)DNLID_MAX; }
  /**
   * \brief check if the DNLInstanceFull is top.
   * \return true if the DNLInstanceFull is top.
   */
  bool isTop() const { return parent_ == (DNLID)DNLID_MAX; }
  const std::pair<DNLID, DNLID>& getChildren() const {
    return childrenIndexes_;
  }
  /**
   * \brief check if the DNLInstanceFull is leaf.
   * \return true if the DNLInstanceFull is leaf.
   */
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
  /**
   * \brief Create a DNLTerminalFull.
   * \param id DNL ID of the DNLTerminalFull.
   */
  DNLTerminalFull(DNLID id);
  /**
   * \brief Create a DNLTerminalFull.
   * \param DNLInstID DNL ID of the DNLInstanceFull.
   * \param terminal the SNLInstTerm of the DNLTerminalFull.
   * \param id DNL ID of the DNLTerminalFull.
   */
  DNLTerminalFull(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id);
  /**
   * \brief Create a DNLTerminalFull.
   * \param DNLInstID DNL ID of the DNLInstanceFull.
   * \param terminal the SNLBitTerm of the DNLTerminalFull.
   * \param id DNL ID of the DNLTerminalFull.
   */
  DNLTerminalFull(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id);
  /**
   * \brief display content of the DNLTerminalFull.
   */
  DNLID getID() const;
  /**
   * \brief get the SNLInstTerm of the DNLTerminalFull.
   * \return the SNLInstTerm of the DNLTerminalFull.
   */
  SNLInstTerm* getSnlTerm() const;
  /**
   * \brief get the SNLBitTerm of the DNLTerminalFull.
   * \return the SNLBitTerm of the DNLTerminalFull.
   */
  SNLBitTerm* getSnlBitTerm() const;
  /**
   * \brief get the DNLInstanceFull of the DNLTerminalFull.
   * \return the DNLInstanceFull of the DNLTerminalFull.
   */
  const DNLInstanceFull& getDNLInstance() const;
  /**
   * \brief check if the DNLTerminalFull is null.
   * \return true if the DNLTerminalFull is null.
   */
  bool isNull() const { return id_ == (DNLID)DNLID_MAX; }
  /**
   * \brief set the DNL ID of the DNLTerminalFull.
   * \param id the DNL ID of the DNLTerminalFull.
   */
  void setIsoID(DNLID isoID);
  /**
   * \brief get the DNL ID of the DNLTerminalFull.
   * \return the DNL ID of the DNLTerminalFull.
   */
  DNLID getIsoID() const;
  /**
   * \brief check if the DNLTerminalFull is top port.
   * \return true if the DNLTerminalFull is top port.
   */
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
  /**
   * \brief clear the DNLIso.
   */
  void clear() {
    drivers_.clear();
    readers_.clear();
  }
  /**
   * \brief set the DNL ID of the DNLIso.
   * \param id the DNL ID of the DNLIso.
   */
  void setId(DNLID id) { id_ = id; }
  /**
   * \brief add a driver to the DNLIso.
   * \param driver the DNL ID of the driver.
   */
  virtual void addDriver(DNLID driver);
  /**
   * \brief add a reader to the DNLIso.
   * \param reader the DNL ID of the reader.
   */
  virtual void addReader(DNLID reader);
  /**
   * \brief add a hier term to the DNLIso.
   * \param hier the DNL ID of the hier term.
   */
  virtual void addHierTerm(DNLID hier) {}
  /**
   * \brief add a net to the DNLIso.
   * \param net the SNLBitNet of the net.
   */
  virtual void addNet(SNLBitNet* net) {}
  /**
   * \brief display content of the DNLIso.
   */
  virtual void display(std::ostream& stream = std::cout) const;
  /**
   * \brief get the DNL ID of the DNLIso.
   * \return the DNL ID of the DNLIso.
   */
  virtual DNLID getIsoID() const { return id_; }
  /**
   * \brief get the drivers of the DNLIso.
   * \return the drivers of the DNLIso.
   */
  virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getDrivers()
      const {
    return drivers_;
  }
  /**
   * \brief get the readers of the DNLIso.
   * \return the readers of the DNLIso.
   */
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
  /**
   * \brief Create a DNLIsoDB.
   */
  DNLIsoDB();
  /**
   * \brief display content of the DNLIsoDB.
   */
  void display() const;
  /**
   * \brief add a DNLIso to the DNLIsoDB.
   * \return the added DNLIso.
   */
  DNLIso& addIso();
  /**
   * \brief get the DNLIso from the DNL ID.
   * \param isoID the DNL ID of the DNLIso.
   * \return the DNLIso of the isoID.
   */
  DNLIso& getIsoFromIsoID(DNLID isoID) { return isos_[isoID]; }
  /**
   * \brief get the DNLIso from the DNL ID.
   * \param isoID the DNL ID of the DNLIso.
   * \return the DNLIso of the isoID.
   */
  const DNLIso& getIsoFromIsoIDconst(DNLID isoID) const { if (isoID == DNLID_MAX) {return isos_.back();} return isos_[isoID]; }
  /**
   * \brief get the number of DNLIso in the DNLIsoDB.
   * \return the number of DNLIso in the DNLIsoDB.
   */
  size_t getNumIsos() const { return isos_.size() - 1/* due to null iso*/; }
  /**
   * \brief get the number of non empty DNLIso in the DNLIsoDB.
   * \return the number of non empty DNLIso in the DNLIsoDB.
   */
  size_t getNumNonEmptyIsos() const { return isos_.size() - 1/* due to null iso*/ - shadowIsos_.size(); }
  /**
   * \brief get the number of DNLIso in the DNLIsoDB.
   * \return the number of DNLIso in the DNLIsoDB.
   */
  std::vector<DNLID> getFullIso(DNLID);
  /**
   * \brief clear the DNLIsoDB.
   */
  void emptyIsos() { isos_.clear();}
  /**
   * \brief add a shadow DNLIso to the DNLIsoDB.
   * \param isoid the DNL ID of the shadow DNLIso.
   */
  void makeShadow(DNLID isoid) { getIsoFromIsoID(isoid).clear(); shadowIsos_.push_back(isoid); }
  /**
   * \brief add a constant 0 DNLIso to the DNLIsoDB.
   * \param isoid the DNL ID of the constant 0 DNLIso.
   */
  void addConstant0Iso(DNLID isoid) { constant0Isos_.insert(isoid); }
  /**
   * \brief get the constant 0 DNLIso in the DNLIsoDB.
   * \return the constant 0 DNLIso in the DNLIsoDB.
   */
  const std::set<DNLID>& getConstant0Isos() const { return constant0Isos_; }
  /**
   * \brief add a constant 1 DNLIso to the DNLIsoDB.
   * \param isoid the DNL ID of the constant 1 DNLIso.
   */
  void addConstant1Iso(DNLID isoid) { constant1Isos_.insert(isoid); }
  /**
   * \brief get the constant 1 DNLIso in the DNLIsoDB.
   * \return the constant 1 DNLIso in the DNLIsoDB.
   */
  const std::set<DNLID>& getConstant1Isos() const { return constant1Isos_; }
 private:
  std::vector<DNLIso, tbb::scalable_allocator<DNLIso>> isos_;
  std::set<DNLID> constant0Isos_;
  std::set<DNLID> constant1Isos_;
  std::vector<DNLID> shadowIsos_;
};

template <class DNLInstance, class DNLTerminal>
class DNLIsoDBBuilder {
 public:
  /**
   * \brief Create a DNLIsoDBBuilder.
   * \param db the DNLIsoDB to build.
   * \param dnl the DNL to build the DNLIsoDB.
   */
  DNLIsoDBBuilder(DNLIsoDB& db, const DNL<DNLInstance, DNLTerminal>& dnl);
  /**
   * \brief treat the driver of the DNLTerminal.
   * \param term the DNLTerminal to treat.
   * \param DNLIso the DNLIso to update.
   * \param visitedDB the visited terms.
   * \param updateReadersIsoID update the readers iso ID.
   * \param updateDriverIsoID update the driver iso ID.
   * \param updateConst update the constant iso.
   */
  void treatDriver(const DNLTerminal& term, DNLIso& DNLIso, visited& visitedDB, 
  bool updateReadersIsoID = false, bool updateDriverIsoID = false, bool updateConst = false);
  /**
   * \brief process the DNLIsoDBBuilder.
   */
  void process();
 private:
  /**
   * \brief register a constant 0 DNLIso to the DNLIsoDB.
   * \param isoid the DNL ID of the DNLIso.
   */
  void addConstantIso0(DNLID iso) { db_.addConstant0Iso(iso); }
  /**
   * \brief register a constant 1 DNLIso to the DNLIsoDB.
   * \param isoid the DNL ID of the DNLIso.
   */
  void addConstantIso1(DNLID iso) { db_.addConstant1Iso(iso); }
  /**
   * \brief add a DNLIso to the DNLIsoDB.
   * \return the added DNLIso.
   */
  DNLIso& addIsoToDB() { return db_.addIso(); }
  DNLIsoDB& db_;
  DNL<DNLInstance, DNLTerminal> dnl_;
};

template <class DNLInstance, class DNLTerminal>
class DNL {
 public:
  /**
   * \brief Create a DNL.
   * \param top the SNLDesign of the DNL.
   */
  DNL(const SNLDesign* top);
  /**
   * \brief process the DNL.
   */
  void process();
  /**
   * \brief display content of the DNL.
   */
  void display() const;
  /**
     * \brief get the custom iso for isoid.
     * \param dnlIsoId the DNL ID of the DNLIso.
     * \param DNLIso the DNLIso to update.
   */
  void getCustomIso(DNLID dnlIsoId, DNLIso& DNLIso) const;
  /**
   * \brief get the DNLInstances of the DNL.
   * \return the DNLInstances of the DNL.
   */
  const std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>>&
  getDNLInstances() const {
    return DNLInstances_;
  }
  /**
   * \brief get the leaves of the DNL.
   * \return the DNLInstances leaves of the DNL.
   */
  const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getLeaves() {
    return leaves_;
  }
  /**
   * \brief get the DNLTerms of the DNL.
   * \return the DNLTerms of the DNL.
   */
  const std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>>&
  getDNLTerms() const {
    return DNLTerms_;
  }
  /**
   * \brief get the DNLTerminal from the DNL ID.
   * \param id the DNL ID of the DNLTerminal.
   * \return the DNLTerminal of the id.
   */
  const DNLTerminal& getDNLTerminalFromID(DNLID id) const {
    if (id == DNLID_MAX) {
      return DNLTerms_.back();
    }
    return DNLTerms_[id];
  }
  /**
   * \brief get the DNLTerminal from the DNL ID.
   * \param id the DNL ID of the DNLTerminal.
   * \return the DNLTerminal of the id.
   */
  DNLTerminal& getNonConstDNLTerminalFromID(DNLID id) { 
    if (id == DNLID_MAX) {
      return DNLTerms_.back();
    }
    return DNLTerms_[id]; 
  }
  /**
   * \brief get the DNLInstance from the DNL ID.
   * \param id the DNL ID of the DNLInstance.
   * \return the DNLInstance of the id.
   */
  const DNLInstance& getDNLInstanceFromID(DNLID id) const {
    if (id == DNLID_MAX) {
      return DNLInstances_.back();
    }
    return DNLInstances_[id];
  }
  /**
   * \brief get the DNLInstance from the DNL ID.
   * \param id the DNL ID of the DNLInstance.
   * \return the DNLInstance of the id.
   */
  DNLInstance& getNonConstDNLInstanceFromID(DNLID id) {
    if (id == DNLID_MAX) {
      return DNLInstances_.back();
    }
    return DNLInstances_[id];
  }
  const DNLTerminal& getDNLNullTerminal() const { return DNLTerms_.back(); }
  /**
   * \brief get the number of terms in the DNL.
   * \return the number of terms in the DNL.
   */
  DNLID getNBterms() const { return DNLTerms_.size() - 1; }
  const DNLInstance& getDNLNullInstance() const { return DNLInstances_.back(); }
  /**
   * \brief get the DNLIsoDB of the DNL.
   * \return the DNLIsoDB of the DNL.
   */
  const DNLIsoDB& getDNLIsoDB() const { return fidb_; }
  /**
   * \brief get number of leaves under parent DNLinstance.
   * \param parent the DNL ID of the parent DNLInstance.
   * \return the number of leaves under parent DNLinstance.
   */
  std::vector<DNLID> getLeavesUnder(DNLID parent) const;
  /**
   * \brief get the top DNLInstance of the DNL.
   * \return the top DNLInstance of the DNL.
   */
  const DNLInstance& getTop() const { return DNLInstances_[0]; }
  /**
   * \brief check if the DNLInstance is child of the parent DNLInstance.
   * \param parent the DNL ID of the parent DNLInstance.
   * \param child the DNL ID of the child DNLInstance.
   * \return true if the DNLInstance is child of the parent DNLInstance.
   */
  bool isInstanceChild(DNLID parent, DNLID child) const;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> getLeaves() const {
    return leaves_;
  }
  void initTermId2isoId() {
    termId2isoId_ = std::vector<DNLID>(DNLTerms_.size(), DNLID_MAX);
  }
  /**
   * \brief set the iso ID for the term ID.
   * \param isoId the DNL ID of the DNLIso.
   * \param termId the DNL ID of the DNLTerminal.
   */
  void setIsoIdforTermId(DNLID isoId, DNLID termId) {
    assert(termId < termId2isoId_.size());
    termId2isoId_[termId] = isoId;
  }
  /**
   * \brief get the iso ID for the term ID.
   * \param termId the DNL ID of the DNLTerminal.
   * \return the DNL ID of the DNLIso.
   */
  DNLID getIsoIdfromTermId(DNLID termId) const {
    if (termId < termId2isoId_.size()) {
      return termId2isoId_[termId];
    }
    return DNLID_MAX;
  }
  /**
   * \brief get the top SNLDesign of the DNL.
   * \return the top SNLDesign of the DNL.
   */
  const SNLDesign* getTopDesign() const { return top_; }
  
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
