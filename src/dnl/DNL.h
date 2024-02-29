// SPDX-FileCopyrightText: 2024 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <iostream>
#include <set>
#include <stack>
#include <vector>
#include "SNLUniverse.h"
#include "tbb/scalable_allocator.h"

using namespace naja::SNL;

namespace naja {
namespace SNL {
    class SNLBitNet;
}
} 

namespace naja {
namespace DNL {

typedef size_t DNLID;
constexpr DNLID DNLID_MAX = ((DNLID)-1);

class DNL;
class DNLTerminal;

class DNLInstance {
 public:
    DNLInstance() = default;
    DNLInstance(const SNLInstance* instance, DNLID id, DNLID parent);
    void display() const;
    DNLID getID() const;
    DNLID getParentID() const;
    const DNLInstance& getParentInstance() const;
    const SNLInstance* getSNLInstance() const;
    const SNLDesign* getSNLModel() const;
    void setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes);
    void setChildrenIndexes(const std::pair<DNLID, DNLID>& childrenIndexes);
    const DNLInstance& getChildInstance(const SNLInstance* snlInst) const;
    const DNLTerminal& getTerminal(const SNLInstTerm* snlTerm) const;
    const DNLTerminal& getTerminalFromBitTerm(const SNLBitTerm* snlTerm) const;
    const std::pair<DNLID, DNLID>& getTermIndexes() const {
        return termsIndexes_;
    }
    bool isNull() const { return id_ == (DNLID) DNLID_MAX; }
    bool isTop() const { return parent_ == (DNLID) DNLID_MAX; }
    const std::pair<DNLID, DNLID>& getChildren() const {
        return childrenIndexes_;
    }
    bool isLeaf() const {
        return childrenIndexes_.first == childrenIndexes_.second;
    }

 private:
    std::pair<DNLID, DNLID> childrenIndexes_;
    const SNLInstance* instance_ { nullptr };
    DNLID id_ = DNLID_MAX;
    DNLID parent_ = DNLID_MAX;
    std::pair<DNLID, DNLID> termsIndexes_;
};

class DNLTerminal {
 public:
    DNLTerminal(DNLID id);
    DNLTerminal(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id);
    DNLTerminal(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id);
    DNLID getID() const;
    SNLInstTerm* getSnlTerm() const;
    SNLBitTerm* getSnlBitTerm() const;
    const DNLInstance& getDNLInstance() const;
    bool isNull() const { return id_ == (DNLID)DNLID_MAX; }
    void setIsoID(DNLID isoID);
    DNLID getIsoID() const;
    bool isTopPort() const { return _terminal == nullptr; }

 private:
    SNLInstTerm* _terminal { nullptr };
    SNLBitTerm* _bitTerminal { nullptr };
    DNLID _DNLInstID = DNLID_MAX;
    DNLID id_ = DNLID_MAX;
    
};

class DNLIso {
 public:
    DNLIso(DNLID id);
    virtual void addDriver(DNLID driver);
    virtual void addReader(DNLID reader);
    virtual void addHierTerm(DNLID hier) {}
    virtual void addNet(SNLBitNet* net) {}
    
    virtual void display(std::ostream& stream = std::cout) const;
    virtual DNLID getIsoID() const { return id_; }
    virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getDrivers() const {
        return drivers_;
    }
    virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getReaders() const {
        return readers_;
    }

 private:
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> drivers_;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> readers_;
    DNLID id_;
    bool isNull_ = true;
};

class DNLComplexIso : public DNLIso {
 public:
    DNLComplexIso(DNLID id) : DNLIso(id) {}
    void addNet(SNLBitNet* net) { _nets.insert(net); }
    const std::set<SNLBitNet*>& getNets() const { return _nets; }
    void addHierTerm(DNLID hier) { _hierTerms.push_back(hier); }
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
    const DNLIso& getIsoFromIsoIDconst(DNLID isoID) const { return isos_[isoID]; }
    DNLID getNumIsos() const { return isos_.size(); }
    std::vector<DNLID> getFullIso(DNLID);
 private:
    std::vector<DNLIso, tbb::scalable_allocator<DNLIso>> isos_;
};

class DNLIsoDBBuilder {
 public:
    DNLIsoDBBuilder(DNLIsoDB& db);

    void process();

 private:
    DNLIso& addIsoToDB() { return db_.addIso(); }
    void treatDriver(const DNLTerminal& term, DNLIso& DNLIso);
    DNLIsoDB& db_;
    
    std::vector<bool> visited_;
};

class DNL {
 public:
    static DNL* create();
    static DNL* get();
    static void destroy();

    void display() const;
    const std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>>&
    getDNLInstances() const {
        return DNLInstances_;
    }
    const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getLeaves() {
        return leaves_;
    }
    const std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>>&
    getDNLTerms() {
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
    DNLID getNBterms() const { return DNLTerms_.size(); }
    const DNLInstance& getDNLNullInstance() const { return DNLInstances_.back(); }
    const DNLIsoDB& getDNLIsoDB() const { return fidb_; }
    std::vector<DNLID> getLeavesUnder(DNLID parent) const;
    const DNLInstance& getTop() const { return DNLInstances_[0]; }
    bool isInstanceChild(DNLID parent, DNLID child) const;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> getLeaves() const {
        return leaves_;
    }
    void initTermId2isoId() {
        termId2isoId_ = std::vector<DNLID>(DNLTerms_.size());
    }
    void setIsoIdforTermId(DNLID isoId, DNLID termId) {
        assert(termId < termId2isoId_.size());
        termId2isoId_[termId] = isoId;
    }
    DNLID getIsoIdfromTermId(DNLID termId) const {
        assert(termId < termId2isoId_.size());
        return termId2isoId_[termId];
    }

 private:
    DNL(const SNLDesign* top);
    void process();
    std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>> DNLInstances_;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> leaves_;
    const SNLDesign* top_;
    std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>> DNLTerms_;
    std::vector<DNLID> termId2isoId_;
    DNLIsoDB fidb_;
};
} 
}
