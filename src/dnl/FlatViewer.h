#include "SNLUniverse.h"
#include <stack> 
#include <vector> 
#include "tbb/scalable_allocator.h"
#include <iostream>
#include <fstream>
#include <set>

using namespace naja::SNL;
namespace naja { namespace SNL {
    class SNLBitNet;
}
}
class FlatViewer;
class FlatTerminal;

class FlatInstance {
    public:
        FlatInstance(FlatViewer& fv);
        FlatInstance(const SNLInstance* instance, size_t id, size_t parent,  FlatViewer& fv);
        void display() const;
        size_t getID() const;
        size_t getParentID() const;
        const FlatInstance& getParentInstance() const;
        //const std::vector<const SNLInstance*>& getFullInstanceBranch() const;
        const SNLInstance* getSNLInstance() const;
        void setTermsIndexes(const std::pair<size_t, size_t>& termsIndexes);
        void setChildrenIndexes(const std::pair<size_t, size_t>& childrenIndexes);
        const FlatInstance& getChildInstance(const SNLInstance* snlInst) const;
        const FlatTerminal& getTerminal(const SNLInstTerm* snlTerm) const;
        const FlatTerminal& getTerminal(const SNLBitTerm* snlTerm) const;
        const std::pair<size_t, size_t>& getTermIndexes() const { return _termsIndexes; }
        bool isNull() const;
        bool isTop() const { return _parent == 0; }
        const std::pair<size_t, size_t>& getChildren() const { return _childrenIndexes; }
        std::pair<size_t, size_t> _childrenIndexes;
        bool isLeaf() const { return _childrenIndexes.first == _childrenIndexes.second; }
    private:
        const SNLInstance* _instance = nullptr;
        size_t _id;
        size_t _parent;
        std::pair<size_t, size_t> _termsIndexes;
        const FlatViewer& _fv;
	    bool _isNull = true;

}; 

//FlatLeafInstance instance

//FlatHierInstsance -> minimal entry for parents 

class FlatTerminal {
public:
    FlatTerminal(FlatViewer& fv, size_t id);
    FlatTerminal(size_t flatInstID,  SNLInstTerm* terminal, size_t id,  FlatViewer& fv);
    size_t getID() const;
    SNLInstTerm* getSnlTerm() const;
    const FlatInstance& getFlatInstance() const;
	bool isNull() const;
    void setIsoID(size_t isoID) { printf("1\n");assert(_isoID == -1); _isoID = isoID; }
    size_t getIsoID() const { return _isoID; }

private:
    SNLInstTerm* _terminal;
    size_t _flatInstID;
    size_t _id;
    size_t _isoID = -1;
    const FlatViewer& _fv;
    bool _isNull = true;
};

class FlatIso {
public:
    //FlatIso();
    FlatIso(size_t id, const FlatViewer& fv);
    void addDriver(size_t driver);
    void addReader(size_t reader);
    void addNet(SNLBitNet* net) { _nets.insert(net); }
    const std::set<SNLBitNet*>& getNets() const { return _nets; }
    void display(std::ostream& stream = std::cout) const;
    size_t getIsoID() const { return _id; }
    const std::vector<size_t, tbb::scalable_allocator<size_t> >& getDrivers() const { return _drivers; }
    const std::vector<size_t, tbb::scalable_allocator<size_t> >& getReaders() const { return _readers; }

private:
    std::vector<size_t, tbb::scalable_allocator<size_t> > _drivers;
    std::vector<size_t, tbb::scalable_allocator<size_t> > _readers;
    std::set<SNLBitNet*> _nets;
    size_t _id;
   bool _isNull = true;
   const FlatViewer& _fv;
};

class FlatIsoDB {
public:
    FlatIsoDB(const FlatViewer& fv);
    void display() const;
    FlatIso& addIso();
    FlatIso& getIsoFromIsoID(size_t isoID) { return _isos[isoID]; }
    const FlatIso& getIsoFromIsoIDconst(size_t isoID) const { return _isos[isoID]; }
    size_t getNumIsos() const { return _isos.size(); }
private:
    std::vector<FlatIso, tbb::scalable_allocator<FlatIso> > _isos;
    const FlatViewer& _fv;
};

class FlatIsoDBBuilder {
public:
    FlatIsoDBBuilder(FlatIsoDB& db, FlatViewer& fv);

    void process();

    

private:
    FlatIso& addIsoToDB() { return _db.addIso(); }
    void treatDriver(const FlatTerminal& term);
   FlatIsoDB& _db;
	FlatViewer& _fv;
	std::vector<bool> _visited;
    
};

class FlatViewer {

public:

    FlatViewer(const SNLDesign *top);
    void process();
    void display() const;
    const std::vector<FlatInstance, tbb::scalable_allocator<FlatInstance> >& getFlatInstances() const { return _flatInstances; }
    const std::vector<size_t, tbb::scalable_allocator<size_t> >& getLeaves() { return _leaves; }
    //add leaf(top and leaf terms + leaf instances) vector
    //equis of only leaf
    const std::vector<FlatTerminal, tbb::scalable_allocator<FlatTerminal> >& getFlatTerms() { return _flatTerms; }
    const FlatTerminal& getFlatTerminalFromID(size_t id) const { return _flatTerms[id]; }
    FlatTerminal& getFlatTerminalFromID(size_t id) { return _flatTerms[id]; }
    const FlatInstance& getFlatInstanceFromID(size_t id) const { return _flatInstances[id-1]; }
    FlatInstance& getNonConstFlatInstanceFromID(size_t id) { return _flatInstances[id-1/*because top is 0*/]; }
    const FlatTerminal& getFlatNullTerminal() const { return _flatTerms.back(); }
    size_t getNBterms() const { return _flatTerms.size(); }
    const FlatInstance& getFlatNullInstance() const { return _flatInstances.back(); }
    void dumpFlatDotFile(bool keepHierInfo = true) const;
    void dumpDotFile() const;
    void dumpDotFileRec(size_t inst, std::ofstream& myfile, size_t& i) const;
    void dumpPartitionedFlatDotFile(const std::vector<std::vector<size_t> >& partitions) const;
    const FlatIsoDB& getFlatIsoDB() const { return _fidb; }
    std::vector<size_t> getLeavesUnder(size_t parent) const;
    const FlatInstance& getTop() const { return _flatInstances[0]; }
    bool isInstanceChild(size_t parent, size_t child) const;
    std::vector<size_t, tbb::scalable_allocator<size_t> > getLeaves() const { return _leaves; }

private:

    std::vector<FlatInstance, tbb::scalable_allocator<FlatInstance> > _flatInstances;
    std::vector<size_t, tbb::scalable_allocator<size_t> > _leaves;
    //add leaf(top and leaf terms + leaf instances) vector
    //equis of only leaf
    const SNLDesign *_top;
    std::vector<FlatTerminal, tbb::scalable_allocator<FlatTerminal> > _flatTerms;
    FlatIsoDB _fidb;
};

class PathExtractor {
public:
    typedef struct subPath {
        subPath(size_t delay,
            size_t path,
            size_t index) {
                _delay = delay;
                _path = path;
                _index = index;
        }
        size_t _delay = 0;
        size_t _path = 0;
        size_t _index = 0;
    } SubPath;
    PathExtractor(const FlatViewer& fv,  std::vector<size_t>& partLeaves) : _fv(fv), _partLeaves(partLeaves),
        _term2paths(fv.getNBterms()) {};
    void cachePaths();
    void printPaths() const {
        for (const auto& path :_paths) {
            printf("----------------\n");
            for (size_t inst : path.first) {
                printf("%lu -> ", inst);
            }
            printf("----------------\n");
        }
    }
    void printHopHistogram();
private:
    std::vector< std::pair<std::vector<size_t>, SubPath> > _paths;
    std::vector<size_t>& _partLeaves;
    const FlatViewer& _fv;
    std::vector<std::vector<SubPath> > _term2paths;
};