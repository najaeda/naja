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
#define DNLID_MAX ((DNLID)-1)

class DNL;
class DNLTerminal;

class DNLInstance {
 public:
    DNLInstance();
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
        return _termsIndexes;
    }
    bool isNull() const { return _id == (DNLID)DNLID_MAX; }
    bool isTop() const { return _parent == (DNLID)DNLID_MAX;; }
    const std::pair<DNLID, DNLID>& getChildren() const {
        return _childrenIndexes;
    }
    bool isLeaf() const {
        return _childrenIndexes.first == _childrenIndexes.second;
    }

 private:
    std::pair<DNLID, DNLID> _childrenIndexes;
    const SNLInstance* _instance = nullptr;
    DNLID _id = DNLID_MAX;
    DNLID _parent = DNLID_MAX;
    std::pair<DNLID, DNLID> _termsIndexes;
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
    bool isNull() const { return _id == (DNLID)DNLID_MAX; }
    void setIsoID(DNLID isoID);
    DNLID getIsoID() const;
    bool isTopPort() const { return _terminal == nullptr; }

 private:
    SNLInstTerm* _terminal = nullptr;
    SNLBitTerm* _bitTerminal = nullptr;
    DNLID _DNLInstID = DNLID_MAX;
    DNLID _id = DNLID_MAX;
    
};

class DNLIso {
 public:
    DNLIso(DNLID id);
    virtual void addDriver(DNLID driver);
    virtual void addReader(DNLID reader);
    virtual void addHierTerm(DNLID hier) {}
    virtual void addNet(SNLBitNet* net) {}
    
    virtual void display(std::ostream& stream = std::cout) const;
    virtual DNLID getIsoID() const { return _id; }
    virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getDrivers() const {
        return _drivers;
    }
    virtual const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getReaders() const {
        return _readers;
    }

 private:
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> _drivers;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> _readers;
    DNLID _id;
    bool _isNull = true;
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
    DNLIso& getIsoFromIsoID(DNLID isoID) { return _isos[isoID]; }
    const DNLIso& getIsoFromIsoIDconst(DNLID isoID) const { return _isos[isoID]; }
    DNLID getNumIsos() const { return _isos.size(); }
    std::vector<DNLID> getFullIso(DNLID);
 private:
    std::vector<DNLIso, tbb::scalable_allocator<DNLIso>> _isos;
};

class DNLIsoDBBuilder {
 public:
    DNLIsoDBBuilder(DNLIsoDB& db);

    void process();

 private:
    DNLIso& addIsoToDB() { return _db.addIso(); }
    void treatDriver(const DNLTerminal& term, DNLIso& DNLIso);
    DNLIsoDB& _db;
    
    std::vector<bool> _visited;
};

class DNL {
 public:
    static DNL* create();
    static DNL* get();
    static void destroy();

    void display() const;
    const std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>>&
    getDNLInstances() const {
        return _DNLInstances;
    }
    const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getLeaves() {
        return _leaves;
    }
    const std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>>&
    getDNLTerms() {
        return _DNLTerms;
    }
    const DNLTerminal& getDNLTerminalFromID(DNLID id) const {
        return _DNLTerms[id];
    }
    DNLTerminal& getDNLTerminalFromID(DNLID id) { return _DNLTerms[id]; }
    const DNLInstance& getDNLInstanceFromID(DNLID id) const {
        return _DNLInstances[id];
    }
    DNLInstance& getNonConstDNLInstanceFromID(DNLID id) {
        return _DNLInstances[id];
    }
    const DNLTerminal& getDNLNullTerminal() const { return _DNLTerms.back(); }
    DNLID getNBterms() const { return _DNLTerms.size(); }
    const DNLInstance& getDNLNullInstance() const { return _DNLInstances.back(); }
    void dumpDNLDotFile(bool keepHierInfo = true) const;
    void dumpDotFile() const;
    void dumpDotFileRec(DNLID inst, std::ofstream& myfile, DNLID& i) const;
    const DNLIsoDB& getDNLIsoDB() const { return _fidb; }
    std::vector<DNLID> getLeavesUnder(DNLID parent) const;
    const DNLInstance& getTop() const { return _DNLInstances[0]; }
    bool isInstanceChild(DNLID parent, DNLID child) const;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> getLeaves() const {
        return _leaves;
    }
    void initTermId2isoId() {
        _termId2isoId = std::vector<DNLID>(_DNLTerms.size());
    }
    void setIsoIdforTermId(DNLID isoId, DNLID termId) {
        assert(termId < _termId2isoId.size());
        _termId2isoId[termId] = isoId;
    }
    DNLID getIsoIdfromTermId(DNLID termId) const {
        assert(termId < _termId2isoId.size());
        return _termId2isoId[termId];
    }

 private:
    DNL(const SNLDesign* top);
    void process();
    std::vector<DNLInstance, tbb::scalable_allocator<DNLInstance>> _DNLInstances;
    std::vector<DNLID, tbb::scalable_allocator<DNLID>> _leaves;
    const SNLDesign* _top;
    std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>> _DNLTerms;
    std::vector<DNLID> _termId2isoId;
    DNLIsoDB _fidb;
};
} 
}
