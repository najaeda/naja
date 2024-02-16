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
}  // namespace naja

namespace naja {
namespace DNL {

typedef size_t DNLID;
#define DNLID_MAX ((DNLID)-1)

class DNL;
class DNLTerminal;

class DNLInstance {

 public:
  DNLInstance(DNL& fv);
  DNLInstance(const SNLInstance* instance, DNLID id, DNLID parent, DNL& fv);
  void display() const;
  DNLID getID() const;
  DNLID getParentID() const;
  const DNLInstance& getParentInstance() const;
  const SNLInstance* getSNLInstance() const;
  const SNLDesign* getSNLModel() const {
    if (_instance) {
      return _instance->getModel();
    } else {
      return SNLUniverse::get()->getTopDesign();
    }
  }
  void setTermsIndexes(const std::pair<DNLID, DNLID>& termsIndexes);
  void setChildrenIndexes(const std::pair<DNLID, DNLID>& childrenIndexes);
  const DNLInstance& getChildInstance(const SNLInstance* snlInst) const;
  const DNLTerminal& getTerminal(const SNLInstTerm* snlTerm) const;
  const DNLTerminal& getTerminalFromBitTerm(const SNLBitTerm* snlTerm) const;
  const std::pair<DNLID, DNLID>& getTermIndexes() const {
    return _termsIndexes;
  }
  bool isNull() const { return _id == (DNLID)DNLID_MAX; }
  bool isTop() const { return _parent == 0; }
  const std::pair<DNLID, DNLID>& getChildren() const {
    return _childrenIndexes;
  }
  std::pair<DNLID, DNLID> _childrenIndexes;
  bool isLeaf() const {
    return _childrenIndexes.first == _childrenIndexes.second;
  }

 private:
  const SNLInstance* _instance = nullptr;
  DNLID _id = DNLID_MAX;
  DNLID _parent = DNLID_MAX;
  std::pair<DNLID, DNLID> _termsIndexes;
  const DNL& _dnl;
};

class DNLTerminal {

 public:

  DNLTerminal(DNL& fv, DNLID id);
  DNLTerminal(DNLID DNLInstID, SNLInstTerm* terminal, DNLID id, DNL& fv);
  DNLTerminal(DNLID DNLInstID, SNLBitTerm* terminal, DNLID id, DNL& fv);
  DNLID getID() const;
  SNLInstTerm* getSnlTerm() const;
  SNLBitTerm* getSnlBitTerm() const;
  const DNLInstance& getDNLInstance() const;
  bool isNull() const { return _id == (DNLID)DNLID_MAX; }
  void setIsoID(DNLID isoID);
  DNLID getIsoID() const;
  bool isTopPort() const { return _terminal == nullptr; }
 private:
  bool isInstTerm = false;
  SNLInstTerm* _terminal = nullptr;
  SNLBitTerm* _bitTerminal = nullptr;
  DNLID _DNLInstID = DNLID_MAX;
  DNLID _id = DNLID_MAX;
  DNL& _dnl;
};

class DNLIso {
 public:
  DNLIso(DNLID id, const DNL& fv);
  void addDriver(DNLID driver);
  void addReader(DNLID reader);
  void addNet(SNLBitNet* net) { _nets.insert(net); }
  const std::set<SNLBitNet*>& getNets() const { return _nets; }
  void display(std::ostream& stream = std::cout) const;
  DNLID getIsoID() const { return _id; }
  const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getDrivers() const {
    return _drivers;
  }
  const std::vector<DNLID, tbb::scalable_allocator<DNLID>>& getReaders() const {
    return _readers;
  }

 private:
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> _drivers;
  std::vector<DNLID, tbb::scalable_allocator<DNLID>> _readers;
  std::set<SNLBitNet*> _nets;
  DNLID _id;
  bool _isNull = true;
  const DNL& _dnl;
};

class DNLIsoDB {
 public:
  DNLIsoDB(const DNL& fv);
  void display() const;
  DNLIso& addIso();
  DNLIso& getIsoFromIsoID(DNLID isoID) { return _isos[isoID]; }
  const DNLIso& getIsoFromIsoIDconst(DNLID isoID) const { return _isos[isoID]; }
  DNLID getNumIsos() const { return _isos.size(); }

 private:
  std::vector<DNLIso, tbb::scalable_allocator<DNLIso>> _isos;
  const DNL& _dnl;
};

class DNLIsoDBBuilder {
 public:
  DNLIsoDBBuilder(DNLIsoDB& db, DNL& fv);

  void process();

 private:
  DNLIso& addIsoToDB() { return _db.addIso(); }
  void treatDriver(const DNLTerminal& term);
  DNLIsoDB& _db;
  DNL& _dnl;
  std::vector<bool> _visited;
};

class DNL {
 public:
  
   ///\return a created singleton DNL or an error if it exists already
  static DNL* create();
  ///\return the singleron DNL or null if it does not exist.
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
  // add leaf(top and leaf terms + leaf instances) vector
  // equis of only leaf
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
    return _DNLInstances[id /*because top is 0*/];
  }
  const DNLTerminal& getDNLNullTerminal() const { return _DNLTerms.back(); }
  DNLID getNBterms() const { return _DNLTerms.size(); }
  const DNLInstance& getDNLNullInstance() const { return _DNLInstances.back(); }
  void dumpDNLDotFile(bool keepHierInfo = true) const;
  void dumpDotFile() const;
  void dumpDotFileRec(DNLID inst, std::ofstream& myfile, DNLID& i) const;
  void dumpPartitionedDNLDotFile(
      const std::vector<std::vector<DNLID>>& partitions) const;
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
  // add leaf(top and leaf terms + leaf instances) vector
  // equis of only leaf
  const SNLDesign* _top;
  std::vector<DNLTerminal, tbb::scalable_allocator<DNLTerminal>> _DNLTerms;
  std::vector<DNLID> _termId2isoId;
  DNLIsoDB _fidb;
};

class PathExtractor {
 public:
  typedef struct subPath {
    subPath(DNLID delay, DNLID path, DNLID index) {
      _delay = delay;
      _path = path;
      _index = index;
    }
    DNLID _delay = 0;
    DNLID _path = 0;
    DNLID _index = 0;
  } SubPath;
  PathExtractor(const DNL& fv, std::vector<DNLID>& partLeaves)
      : _dnl(fv), _partLeaves(partLeaves), _term2paths(fv.getNBterms()){};
  void cachePaths();
  void printPaths() const {
    for (const auto& path : _paths) {
      printf("----------------\n");
      for (DNLID inst : path.first) {
        printf("%lu -> ", inst);
      }
      printf("----------------\n");
    }
  }
  void printHopHistogram();

 private:
  std::vector<std::pair<std::vector<DNLID>, SubPath>> _paths;
  std::vector<DNLID>& _partLeaves;
  const DNL& _dnl;
  std::vector<std::vector<SubPath>> _term2paths;
};
}  // namespace DNL
}  // namespace naja