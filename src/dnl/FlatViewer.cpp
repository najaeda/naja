#include "SNLUniverse.h"
#include <stack>
#include <vector>
#include "FlatViewer.h"
#include "SNLInstTerm.h"
#include "SNLInstance.h"
#include "SNLBitNet.h"
#include "SNLBitTerm.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include <iostream>
#include <fstream>
#include <set>
#include <limits>

using namespace naja::SNL;

#define DEBUG_PRINTS false

FlatInstance::FlatInstance(FlatViewer &fv) : _fv(fv)
{
}
FlatInstance::FlatInstance(const SNLInstance *instance, size_t id, size_t parent, FlatViewer &fv) : 
    _instance(instance), _id(id), _parent(parent), _fv(fv), _isNull(false)
{
    // postProcess();
}

void FlatInstance::display() const
{
    printf("fi %lu %s\n", getID(), getSNLInstance()->getString().c_str());
    for (size_t term = getTermIndexes().first;
         term < getTermIndexes().second; term++)
    {
        printf("- ft %lu %d %s\n", term,
               (int)_fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection(), _fv.getFlatTerminalFromID(term).getSnlTerm()->getString().c_str());
    }
}

const FlatInstance &FlatInstance::getParentInstance() const { return _fv.getFlatInstanceFromID(_parent); };
size_t FlatInstance::getID() const { return _id; }
size_t FlatInstance::getParentID() const { return _parent; }
// const std::vector<const SNLInstance*>& getFullInstanceBranch() const { return _fullInstanceBranch; }
const SNLInstance *FlatInstance::getSNLInstance() const { return _instance; }
void FlatInstance::setTermsIndexes(const std::pair<size_t, size_t> &termsIndexes) { _termsIndexes = termsIndexes; }
void FlatInstance::setChildrenIndexes(const std::pair<size_t, size_t> &childrenIndexes) { _childrenIndexes = childrenIndexes; }
const FlatInstance &FlatInstance::getChildInstance(const SNLInstance *snlInst) const
{
#ifdef DEBUG_PRINTS
    printf("%lu %lu %lu %d\n", _id, _childrenIndexes.first, _childrenIndexes.second, (int)isTop());
#endif
    for (size_t child = _childrenIndexes.first + 1; child <= _childrenIndexes.second; child++)
    {
#ifdef DEBUG_PRINTS
        printf("%p %p\n", snlInst, _fv.getFlatInstanceFromID(child).getSNLInstance());
#endif
        if (_fv.getFlatInstanceFromID(child).getSNLInstance() == snlInst)
        {
            return _fv.getFlatInstanceFromID(child);
        }
    }
#ifdef DEBUG_PRINTS
    printf("null inst\n");
#endif
    return _fv.getFlatNullInstance();
}
const FlatTerminal &FlatInstance::getTerminal(const SNLInstTerm *snlTerm) const
{
    for (size_t term = _termsIndexes.first; term < _termsIndexes.second; term++)
    {
        if (_fv.getFlatTerminalFromID(term).getSnlTerm() == snlTerm)
        {
            return _fv.getFlatTerminalFromID(term);
        }
    }
#ifdef DEBUG_PRINTS
    printf("null term\n");
#endif
    return _fv.getFlatNullTerminal();
}
const FlatTerminal &FlatInstance::getTerminal(const SNLBitTerm *snlTerm) const
{
    for (size_t term = _termsIndexes.first; term < _termsIndexes.second; term++)
    {
        if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getTerm() == snlTerm)
        {
#ifdef DEBUG_PRINTS
            printf("return %lu %s %s\n", term, _fv.getFlatTerminalFromID(term).getSnlTerm()->getString().c_str(),
                   _fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection().getString().c_str());
#endif
            return _fv.getFlatTerminalFromID(term);
        }
    }
    assert(false);
#ifdef DEBUG_PRINTS
    printf("return null terminal\n");
#endif
    return _fv.getFlatNullTerminal();
}
bool FlatInstance::isNull() const { return isNull(); }

// FlatLeafInstance instance

// FlatHierInstsance -> minimal entry for parents

FlatTerminal::FlatTerminal(FlatViewer &fv, size_t id) : _fv(fv), _id(id){};
FlatTerminal::FlatTerminal(size_t flatInstID, SNLInstTerm *terminal, size_t id, FlatViewer &fv) : _flatInstID(flatInstID), _terminal(terminal), _id(id), _fv(fv), _isNull(false){};
size_t FlatTerminal::getID() const { return _id; }
SNLInstTerm *FlatTerminal::getSnlTerm() const { return _terminal; }
const FlatInstance &FlatTerminal::getFlatInstance() const
{
    return _fv.getFlatInstanceFromID(_flatInstID);
}
bool FlatTerminal::isNull() const { return isNull(); }

FlatIso::FlatIso(size_t id, const FlatViewer &fv) : _id(id), _fv(fv){};
void FlatIso::addDriver(size_t driver) { _drivers.push_back(driver); }
void FlatIso::addReader(size_t reader) { _readers.push_back(reader); }
void FlatIso::display(std::ostream &stream) const
{
    for (auto &driver : _drivers)
    {
        stream << "driver instance" << _fv.getFlatTerminalFromID(driver).getSnlTerm()->getInstance()->getName().getString() << std::endl
               << _fv.getFlatTerminalFromID(driver).getSnlTerm()->getInstance()->getDescription() << std::endl;
        ;
        stream << "driver " << _fv.getFlatTerminalFromID(driver).getSnlTerm()->getString() << std::endl;
        stream << "driver " << _fv.getFlatTerminalFromID(driver).getSnlTerm()->getDescription() << std::endl;
    }
    for (auto &reader : _readers)
    {
        stream << "reader instance" << _fv.getFlatTerminalFromID(reader).getSnlTerm()->getInstance()->getName().getString() << std::endl;
        ;
        stream << "reader" << _fv.getFlatTerminalFromID(reader).getSnlTerm()->getString() << std::endl;
        ;
    }
}

FlatIsoDB::FlatIsoDB(const FlatViewer &fv) : _fv(fv) {}

FlatIso &FlatIsoDB::addIso()
{
    _isos.push_back(FlatIso(_isos.size(), _fv));
    return _isos.back();
}

FlatIsoDBBuilder::FlatIsoDBBuilder(FlatIsoDB &db, FlatViewer &fv) : _db(db), _fv(fv) { _visited.resize(_fv.getFlatTerms().size(), false); }

void FlatIsoDBBuilder::process()
{
    // iterate on all leaf drivers
    std::vector<size_t> tasks;
    for (size_t leaf : _fv.getLeaves())
    {
        for (size_t term = _fv.getFlatInstanceFromID(leaf).getTermIndexes().first;
             term < _fv.getFlatInstanceFromID(leaf).getTermIndexes().second; term++)
        {
            if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                FlatIso &flatIso = addIsoToDB();

                flatIso.addDriver(term);
                tasks.push_back(term);
                _fv.getFlatTerminalFromID(term).setIsoID(flatIso.getIsoID());
            }
        }
    }
    if (!getenv("NON_MT_FV"))
    {
        printf("MT\n");
        tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads()); // Explicit number of threads
        tbb::parallel_for(tbb::blocked_range<size_t>(0, tasks.size()),
                          [&](const tbb::blocked_range<size_t> &r)
                          {
                              for (size_t i = r.begin(); i < r.end(); ++i)
                              {
                                  treatDriver(_fv.getFlatTerminalFromID(tasks[i]));
                              }
                          });
    }
    else
    {
        printf("Non MT\n");
        for (auto task : tasks)
        {
            treatDriver(_fv.getFlatTerminalFromID(task));
        }
    }

    printf("num fi %lu\n", _fv.getFlatInstances().size());
    printf("num ft %lu\n", _fv.getFlatTerms().size());
    printf("num leaves %lu\n", _fv.getLeaves().size());
    printf("num isos %lu\n", _db.getNumIsos());
}

void FlatIsoDB::display() const
{
    printf("----------ISODB - BEGIN----------\n");
    for (const FlatIso &iso : _isos)
    {
        printf("----------new iso----------\n");
        iso.display();
    }
    printf("----------ISODB - END----------\n");
}

void FlatIsoDBBuilder::treatDriver(const FlatTerminal &term)
{
#ifdef DEBUG_PRINTS
    printf("leaf -%s\n", term.getSnlTerm()->getInstance()->getName().getString().c_str());
    printf("#############################################################\n");
    printf("treating %lu %p %s %s\n", term.getID(), term.getSnlTerm(),
           term.getSnlTerm()->getString().c_str(), term.getSnlTerm()->getDirection().getString().c_str());
#endif
    assert(term.getSnlTerm()->getInstance()->getModel()->getInstances().empty());
    std::stack<size_t> stack;
    FlatIso &flatIso = _db.getIsoFromIsoID(term.getIsoID());
    // do DFS on the driver
    stack.push(term.getID());
    // Start traversing from driver
    while (!stack.empty())
    {
        size_t id = stack.top();
        stack.pop();
        const FlatTerminal &fterm = _fv.getFlatTerminalFromID(id);
#ifdef DEBUG_PRINTS
        printf("---------------------------------\n");
        printf("visiting '%lu %p %s %s\n", id, fterm.getSnlTerm()->getInstance(), fterm.getSnlTerm()->getString().c_str(), fterm.getSnlTerm()->getDirection().getString().c_str());
#endif
        if (_visited[id])
        {
#ifdef DEBUG_PRINTS
            printf("--visited continue\n");
#endif
            continue;
        }
        if (fterm.getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output && fterm.getFlatInstance().isTop())
        {
#ifdef DEBUG_PRINTS
            printf("--is top output so adding as reader\n");
#endif
            flatIso.addReader(fterm.getID());
            _fv.getFlatTerminalFromID(fterm.getID()).setIsoID(flatIso.getIsoID());
            continue;
        }
        _visited[id] = true;
        SNLBitNet *snlNet = fterm.getSnlTerm()->getNet();
        flatIso.addNet(snlNet);
        const FlatInstance *flatParent = &fterm.getFlatInstance().getParentInstance();
        // Get snl bit net connected to the snl term object of the flat terminal
        bool goDown = false;
        if (!fterm.getSnlTerm()->getInstance()->getModel()->getInstances().empty())
        {
#ifdef DEBUG_PRINTS
            printf("--hier inst\n");
#endif
        }
#ifdef DEBUG_PRINTS
        printf("--term direction %d\n", fterm.getSnlTerm()->getDirection());
        printf("--term direction %s\n", fterm.getSnlTerm()->getDirection().getString().c_str());
#endif
        if (!fterm.getSnlTerm()->getInstance()->getModel()->getInstances().empty() &&
            fterm.getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Input)
        {
            // The current explored terminal is hierarchical input, need to go into the instance(get bit term -> get net)
            snlNet = fterm.getSnlTerm()->getTerm()->getNet();
            flatParent = &fterm.getFlatInstance();
            goDown = true;
#ifdef DEBUG_PRINTS
            printf("--going down\n");
#endif
        }
        else
        {
#ifdef DEBUG_PRINTS
            printf("--same level\n");
#endif
        }
        if (snlNet == nullptr)
            continue;
#ifdef DEBUG_PRINTS
        printf("--visiting net '%p %s\n", snlNet, snlNet->getString().c_str());
#endif
        for (SNLInstTerm *instTerm : snlNet->getInstTerms())
        {
            SNLInstance *termSnlInst = instTerm->getInstance();
#ifdef DEBUG_PRINTS
            printf("--inst %p term %p\n", termSnlInst, instTerm);
#endif
            const FlatTerminal &ftermNew = goDown ? fterm.getFlatInstance().getChildInstance(termSnlInst).getTerminal(instTerm) : fterm.getFlatInstance().getParentInstance().getChildInstance(termSnlInst).getTerminal(instTerm);
#ifdef DEBUG_PRINTS
            printf("--visiting snl  it %lu %p %s %s\n", ftermNew.getID(), termSnlInst, instTerm->getString().c_str(), instTerm->getDirection().getString().c_str());
            printf("--visiting flat it %lu %p %s %s\n", ftermNew.getID(), ftermNew.getSnlTerm(), ftermNew.getSnlTerm()->getString().c_str(), ftermNew.getSnlTerm()->getDirection().getString().c_str());
#endif
            if (termSnlInst->getModel()->getInstances().empty())
            {
                if (ftermNew.getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
                {
                    // if (ftermNew.getID() == term.getID()) continue;
                    if (term.getID() != ftermNew.getID())
                    {
                        assert(false);
                        flatIso.addDriver(ftermNew.getID());
#ifdef DEBUG_PRINTS
                        printf("----add driver\n\n");
#endif
                    }
                    else
                    {
#ifdef DEBUG_PRINTS
                        printf("----original driver -> skipping\n\n");
#endif
                    }
                    // assert(0);
                }
                else
                {
                    flatIso.addReader(ftermNew.getID());
                    _fv.getFlatTerminalFromID(ftermNew.getID()).setIsoID(flatIso.getIsoID());
#ifdef DEBUG_PRINTS
                    printf("----add reader\n\n");
#endif
                }
            }
            else
            {
                stack.push(ftermNew.getID());
#ifdef DEBUG_PRINTS
                printf("----pushing to stuck %s %s\n",
                       ftermNew.getSnlTerm()->getString().c_str(),
                       ftermNew.getSnlTerm()->getDirection().getString().c_str());
#endif
            }
        }
        for (SNLBitTerm *bitTerm : snlNet->getBitTerms())
        {
#ifdef DEBUG_PRINTS
            printf("--visiting bt %p %s %s\n", bitTerm,
                   bitTerm->getString().c_str(), bitTerm->getDirection().getString().c_str());
#endif
        }
        for (SNLBitTerm *bitTerm : snlNet->getBitTerms())
        {
#ifdef DEBUG_PRINTS
            printf("--visiting bt %p %s %s\n", bitTerm, bitTerm->getString().c_str(), bitTerm->getDirection().getString().c_str());
            flatParent->display();
            flatParent->getParentInstance().display();
            printf("--visiting bt %lu %p %s %s\n", flatParent->getTerminal(bitTerm).getID(), bitTerm, bitTerm->getString().c_str(), bitTerm->getDirection().getString().c_str());
#endif
            const FlatTerminal &ftermNew = flatParent->getTerminal(bitTerm);
#ifdef DEBUG_PRINTS
            printf("----pushing to stuck %s %s\n", ftermNew.getSnlTerm()->getString().c_str(), ftermNew.getSnlTerm()->getDirection().getString().c_str());
#endif
            if (_visited[ftermNew.getID()])
            {
#ifdef DEBUG_PRINTS
                printf("--visited continue\n");
#endif
                continue;
            }
            stack.push(ftermNew.getID());
        }
    }
}

FlatViewer::FlatViewer(const SNLDesign *top) : _top(top), _fidb(*this)
{
}

void FlatViewer::dumpDotFile() const
{
    std::ofstream myfile;
    myfile.open("./netlist.dat");
    std::stack<size_t> stack;
    stack.push(0);
    myfile << "digraph " << _top->getName().getString() << " {\n rankdir=LR\n";
    /*for (SNLInstance *inst : _top->getInstances())
    {
        dumpDotFileRec(inst->getModel(), myfile);
    }*/
    size_t i = 0;
    dumpDotFileRec(0, myfile, i);
    myfile << "}";
    myfile.close();
}

void FlatViewer::dumpDotFileRec(size_t inst, std::ofstream &myfile, size_t &i) const
{
    if (getFlatInstanceFromID(inst).getChildren().first == getFlatInstanceFromID(inst).getChildren().second)
    {
        // myfile << getFlatInstanceFromID(inst).getSNLInstance()->getName().getString() << std::endl;;
        myfile << "leaf" << inst << " [shape=record, label=\"" << getFlatInstanceFromID(inst).getSNLInstance()->getName().getString() << "\"];" << std::endl;
        return;
    }

    if (inst != 1)
    {
        myfile << "subgraph cluster_" << i << " {" << std::endl;
        myfile << "label = \""
               << getFlatInstanceFromID(inst).getSNLInstance()->getName().getString() << "\";" << std::endl;
        ;
        i++;
    }
    else
    {
        myfile << "subgraph cluster_" << i << " {" << std::endl;
        myfile << "label = \""
               << "top"
               << "\";" << std::endl;
        i++;
    }
    // for (SNLInstance *inst : toDump->getInstances())
    printf("here %lu %lu\n", getFlatInstanceFromID(inst).getChildren().first + 1,
           getFlatInstanceFromID(inst).getChildren().second);
    for (size_t child = getFlatInstanceFromID(inst).getChildren().first + 1;
         child <= getFlatInstanceFromID(inst).getChildren().second; child++)
    {
        printf("here\n");
        dumpDotFileRec(child, myfile, i);
    }
    myfile
        << "}" << std::endl;
}

void FlatViewer::dumpFlatDotFile(bool keepHierInfo) const
{
    std::ofstream myfile;

    size_t i = 0;
    if (keepHierInfo)
    {

        myfile.open(std::string("./") + _top->getName().getString() + std::string(".dat"));
        myfile << "digraph FlatNetlist {" << std::endl;
        dumpDotFileRec(1, myfile, i);
    }
    else
    {
        myfile.open("./flatNetlist.dat");
        myfile << "digraph FlatNetlist {" << std::endl;
    }

    /*for (size_t leaf : _leaves) {
        myfile << "leaf" << leaf << std::endl;
    }*/
    myfile << std::endl;
    // for (size_t isoId : _fidb.getNumIsos()) {
    for (size_t isoId = 0; isoId < _fidb.getNumIsos(); isoId++)
    {
        const FlatIso &iso = _fidb.getIsoFromIsoIDconst(isoId);
        for (size_t driver : iso.getDrivers())
        {
            for (size_t reader : iso.getReaders())
            {
                myfile << "leaf" << getFlatTerminalFromID(driver).getFlatInstance().getID() << " -> "
                       << "leaf" << getFlatTerminalFromID(reader).getFlatInstance().getID() << std::endl;
                // myfile <<  getFlatInstanceFromID(driver).getSNLInstance()->getName().getString() << " -> "
                //<<  getFlatInstanceFromID(reader).getSNLInstance()->getName().getString()<< std::endl;
            }
        }
    }
    myfile << "}";
}

void FlatViewer::display() const
{
    printf("---------FV--------\n");
    for (const FlatInstance &inst : _flatInstances)
    {
        if (inst.getSNLInstance() == nullptr)
            continue; // top
        printf("fi %lu %s\n", inst.getID(), inst.getSNLInstance()->getString().c_str());
        for (size_t term = inst.getTermIndexes().first;
             term < inst.getTermIndexes().second; term++)
        {
            printf("- ft %lu %d %s\n", term, (int)getFlatTerminalFromID(term).getSnlTerm()->getDirection(), getFlatTerminalFromID(term).getSnlTerm()->getString().c_str());
        }
    }
    _fidb.display();
}

void FlatViewer::process()
{
    std::vector<size_t> stack;
    _flatInstances.push_back(FlatInstance(nullptr, _flatInstances.size() + 1, 0, *this));
    assert(_flatInstances.back().getID() == _flatInstances.size());
    size_t parentId = _flatInstances.back().getID();
    // FlatInstance& flatParent = getNonConstFlatInstanceFromID(_flatInstances.back().getID());
    std::pair<size_t, size_t> childrenIndexes;
    childrenIndexes.first = _flatInstances.back().getID();
    for (auto inst : _top->getInstances())
    {
        _flatInstances.push_back(FlatInstance(inst, _flatInstances.size() + 1, parentId, *this));
        stack.push_back(_flatInstances.back().getID());
        std::pair<size_t, size_t> termIndexes;
        termIndexes.first = _flatTerms.size();
        if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() || inst->getModel()->getInstances().empty())
        {
            _leaves.push_back(_flatInstances.back().getID());
            // printf("leaf %lu -%s\n", _flatInstances.back().getID(), inst->getName().getString().c_str());
        }
        for (auto term : inst->getInstTerms())
        {
            _flatTerms.push_back(FlatTerminal(_flatInstances.back().getID(), term, _flatTerms.size(), *this));
        }
        termIndexes.second = _flatTerms.size();
        _flatInstances.back().setTermsIndexes(termIndexes);
    }
    childrenIndexes.second = _flatInstances.back().getID();
    // printf("%lu %lu\n", childrenIndexes.first, childrenIndexes.second);
    getNonConstFlatInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
    // printf("%lu %lu\n", getNonConstFlatInstanceFromID(parentId)._childrenIndexes.first, getNonConstFlatInstanceFromID(parentId)._childrenIndexes.second);
    while (!stack.empty())
    {
#ifdef DEBUG_PRINTS
        printf("check %p\n", _flatInstances[0].getSNLInstance());
#endif
        const SNLInstance *parent = getNonConstFlatInstanceFromID((stack.back())).getSNLInstance();
        size_t parentId = getNonConstFlatInstanceFromID((stack.back())).getID();
        stack.pop_back();
        std::pair<size_t, size_t> childrenIndexes;
        childrenIndexes.first = _flatInstances.back().getID();
        for (auto inst : parent->getModel()->getInstances())
        {
#ifdef DEBUG_PRINTS
            printf("push %p\n", inst);
            printf("push -%s\n", inst->getName().getString().c_str());
#endif
            // if (!inst) continue;
            _flatInstances.push_back(FlatInstance(inst, _flatInstances.size() + 1, parentId, *this));
#ifdef DEBUG_PRINTS
            printf("-%s\n", inst->getName().getString().c_str());
#endif
            stack.push_back(_flatInstances.back().getID());
            // flatParent->addChild(_flatInstances.back().getID());
            if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() || inst->getModel()->getInstances().empty())
            {
                _leaves.push_back(_flatInstances.back().getID());
#ifdef DEBUG_PRINTS
                printf("leaf %lu -%s\n", _flatInstances.back().getID(),
                       inst->getName().getString().c_str());
#endif
            }
            std::pair<size_t, size_t> termIndexes;
            termIndexes.first = _flatTerms.size();
            for (auto term : inst->getInstTerms())
            {
                _flatTerms.push_back(FlatTerminal(_flatInstances.back().getID(), term, _flatTerms.size(), *this));
#ifdef DEBUG_PRINTS
                printf("term %lu -%s\n", _flatTerms.back().getID(), term->getString().c_str());
#endif
            }
            termIndexes.second = _flatTerms.size();
            _flatInstances.back().setTermsIndexes(termIndexes);
        }
        childrenIndexes.second = _flatInstances.back().getID();
        getNonConstFlatInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
    }
    _flatTerms.push_back(FlatTerminal(*this, _flatTerms.size()));
    _flatInstances.push_back(FlatInstance(*this));
    FlatIsoDBBuilder fidbb(_fidb, *this);
    fidbb.process();
}

void FlatViewer::dumpPartitionedFlatDotFile(const std::vector<std::vector<size_t>> &partitions) const
{
    std::ofstream myfile;

    size_t i = 0;
    myfile.open("./pfn.dat");
    myfile << "digraph FlatNetlist {" << std::endl;
    size_t part = 0;
    for (const std::vector<size_t> &partition : partitions)
    {
        myfile << "subgraph cluster_" << part << " {" << std::endl;
        myfile << "label = \""
               << "part " << part << "\";" << std::endl;
        ;
        for (size_t leaf : partition)
        {
            myfile << "leaf" << leaf << std::endl;
        }
        myfile << "}" << std::endl;
        part++;
    }
    /*for (size_t leaf : _leaves) {
        myfile << "leaf" << leaf << std::endl;
    }*/
    myfile << std::endl;
    // for (size_t isoId : _fidb.getNumIsos()) {
    for (size_t isoId = 0; isoId < _fidb.getNumIsos(); isoId++)
    {
        const FlatIso &iso = _fidb.getIsoFromIsoIDconst(isoId);
        for (size_t driver : iso.getDrivers())
        {
            for (size_t reader : iso.getReaders())
            {
                myfile << "leaf" << getFlatTerminalFromID(driver).getFlatInstance().getID() << " -> "
                       << "leaf" << getFlatTerminalFromID(reader).getFlatInstance().getID() << std::endl;
            }
        }
    }
    myfile << "}";
}

std::vector<size_t> FlatViewer::getLeavesUnder(size_t parent) const
{
    std::vector<size_t> leaves;
    std::stack<size_t> toProcess;
    toProcess.push(parent);
    while (!toProcess.empty())
    {

        const FlatInstance &inst = getFlatInstanceFromID(toProcess.top());
        if (inst.isLeaf())
            leaves.push_back(toProcess.top());
        toProcess.pop();
        for (size_t child = inst.getChildren().first + 1;
             child <= inst.getChildren().second; child++)
        {
            toProcess.push(child);
        }
    }
    return leaves;
}

bool FlatViewer::isInstanceChild(size_t parent, size_t child) const
{
    size_t inst = child;
    if (parent == 1)
    {
        return true;
    }
    while (getFlatInstanceFromID(child).getParentID() != 1)
    {
        if (parent == inst)
        {
            return true;
        }
        inst = getFlatInstanceFromID(child).getParentID();
    }
    return false;
}

void PathExtractor::cachePaths()
{
    std::vector<size_t> sources;
    const std::vector<size_t, tbb::scalable_allocator<size_t>> &leaves = _fv.getLeaves();
    std::vector<bool> visited(_fv.getNBterms(), false);
    size_t criticalDelay = 0;
    printf("leaves number: %lu\n", leaves.size());
    for (size_t leaf : leaves)
    {
        bool source = true;
        for (size_t term = _fv.getFlatInstanceFromID(leaf).getTermIndexes().first;
             term < _fv.getFlatInstanceFromID(leaf).getTermIndexes().second; term++)
        {
            if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection() != SNLTerm::Direction::DirectionEnum::Output && _fv.getFlatTerminalFromID(term).getSnlTerm()->getNet() != nullptr)
            {
                source = false;
            }
        }
        if (source)
        {
            for (size_t term = _fv.getFlatInstanceFromID(leaf).getTermIndexes().first;
                 term < _fv.getFlatInstanceFromID(leaf).getTermIndexes().second; term++)
            {
                sources.push_back(term);
            }
        }
    }
    for (size_t source : sources)
    {
        std::stack<size_t> stack;
        std::vector<size_t> path;
        stack.push(source);
        std::vector<bool> visitedLoops(_fv.getNBterms(), false);
        // std::vector<bool> visited(_fv.getNBterms(), false);
        while (!stack.empty())
        {
            /*printf("--------------\n");
            for (size_t p : path) {
                printf("---%s\n",  _fv.getFlatTerminalFromID(p).getSnlTerm()->getString().c_str());
            }
            printf("path %lu\n", path.size());*/
            if (_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().size() > 1)
            {

                visited[stack.top()] = true;
                stack.pop();
                continue; // MD
            }
            if (_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().size() == 0)
            {

                visited[stack.top()] = true;
                stack.pop();
                continue; // Stub
            }
            assert(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().size() == 1);
            size_t toProcess = stack.top();
            // visited[toProcess] = true;
            if (toProcess != source)
                assert(!path.empty());
            // printf("%s\n",  _fv.getFlatTerminalFromID(toProcess).getSnlTerm()->getString().c_str());
            // printf("%lu\n", toProcess);
            if (!path.empty())
            {
                /*printf("%s\n",  _fv.getFlatTerminalFromID(toProcess).getSnlTerm()->getString().c_str());
                printf("%lu\n", toProcess);
                printf("driver %lu\n", (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));

                printf("path: %lu %s\n", path.back(),  _fv.getFlatTerminalFromID(path.back()).getSnlTerm()->getString().c_str());
                printf("iso: %lu %lu\n", _fv.getFlatTerminalFromID(path.back()).getIsoID()
                    ,_fv.getFlatTerminalFromID(stack.top()).getIsoID());
                printf("inst: %lu %lu\n", _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID(),
                            _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());*/
                assert(_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() ==
                           _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID() ||
                       (path.back() ==
                        (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))));
            }

            stack.pop();
            if (visitedLoops[toProcess] &&
                _fv.getFlatTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Input)
            { // Loop
                if (_fv.getFlatTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    path.pop_back();
                    assert(_fv.getFlatTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                                                _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID())
                    {
                        // printf("path: %lu\n", path.size());
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                    }
                    /*if (_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                        _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID()) {
                        //printf("%lu %lu\n", _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID(),
                        //_fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    }*/
                    assert(_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() ==
                           _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    continue;
                }
                // path.pop_back();//To remove the previous input
                while (!path.empty() && (path.back() !=
                                         (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
                    // printf("path: %lu\n", path.size());
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                }
                assert(path.back() ==
                       (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                continue;
            }

            if (visited[toProcess] &&
                _fv.getFlatTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                size_t hopsInit = 0;
                size_t lastPart = -1;
                for (size_t term : path)
                {
                    size_t inst_id = _fv.getFlatTerminalFromID(term).getFlatInstance().getID();
                    if (lastPart != -1)
                    {
                        if (_partLeaves[inst_id] != lastPart)
                        {
                            hopsInit++;
                            lastPart = _partLeaves[inst_id];
                        }
                    }
                    else
                    {
                        lastPart = _partLeaves[inst_id];
                    }
                }
                for (const PathExtractor::SubPath &toCache : _term2paths[toProcess])
                {
                    std::vector<size_t> pathInst;
                    size_t hops = hopsInit;
                    lastPart = -1;
                    // printf("1 %lu %lu\n", hops, toCache._delay);
                    hops += toCache._delay;
                    size_t hopTemp = 0;

                    for (size_t term : path)
                    {
                        size_t inst_id = _fv.getFlatTerminalFromID(term).getFlatInstance().getID();
                        if (lastPart != -1)
                        {
                            if (_partLeaves[inst_id] != lastPart)
                            {
                                hopTemp++;
                                lastPart = _partLeaves[inst_id];
                            }
                        }
                        else
                        {
                            lastPart = _partLeaves[inst_id];
                        }
                        if (pathInst.empty() || inst_id != pathInst.back())
                        {
                            pathInst.push_back(inst_id);
                            // printf("1 %lu(%lu-%lu) %lu %lu\n", hops - hopTemp, hops, hopTemp, _paths.size(), pathInst.size() - 1);
                            _term2paths[_fv.getFlatTerminalFromID(term).getFlatInstance().getID()].emplace_back(
                                PathExtractor::SubPath(hops - hopTemp, _paths.size(), pathInst.size() - 1));
                        }
                    }
                    // pathInst.insert(pathInst.end(), (*(_paths.begin() + toCache._path)).begin() + toCache._index, (*(_paths.begin() + toCache._path)).end());
                    _paths.emplace_back(std::pair<std::vector<size_t>, SubPath>(pathInst, toCache));
                    // printf("%lu\n", _paths.size());
                    // printf("pi %lu\n", pathInst.size());
                }
                if (stack.empty())
                {
                    break;
                }
                if (path.empty())
                {
                    continue;
                }
                // printf("stack: %lu\n", stack.size());
                // printf("path: %lu\n", path.size());
                // path.pop_back();//To remove the previous input
                if (_fv.getFlatTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    // path.pop_back();
                    assert(_fv.getFlatTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                                                _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID())
                    {
                        // printf("path: %lu\n", path.size());
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                    }
                    /*if (_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                        _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID()) {
                        //printf("%lu %lu\n", _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID(),
                        //_fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    }*/
                    assert(_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() ==
                           _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    // printf("A\n");
                    assert(visited[toProcess]);
                    continue;
                }
                path.pop_back(); // To remove the previous input
                while (!path.empty() && (path.back() !=
                                         (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
                    // printf("path: %lu\n", path.size());
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                }
                // printf("B\n");
                assert(visited[toProcess]);
                assert(path.back() ==
                       (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                continue;
            }
            visited[toProcess] = true;
            visitedLoops[toProcess] = true;
            path.push_back(toProcess);
            bool isSink = false;
            if (_fv.getFlatTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                const FlatIso &iso = _fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(toProcess).getIsoID());
                for (size_t reader : iso.getReaders())
                {
                    isSink = false;
                    stack.push(reader);
                    assert(_fv.getFlatTerminalFromID(reader).getIsoID() == _fv.getFlatTerminalFromID(toProcess).getIsoID());
                }
            }
            else
            {
                isSink = true;
                if (path.size() <= 20 || true)
                {
                    size_t inst_id = _fv.getFlatTerminalFromID(toProcess).getFlatInstance().getID();
                    for (size_t term = _fv.getFlatInstanceFromID(inst_id).getTermIndexes().first;
                         term < _fv.getFlatInstanceFromID(inst_id).getTermIndexes().second; term++)
                    {
                        if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
                        {

                            assert(_fv.getFlatTerminalFromID(term).getFlatInstance().getID() ==
                                   _fv.getFlatTerminalFromID(toProcess).getFlatInstance().getID());
                            if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getNet() != nullptr)
                            {
                                isSink = false;
                                stack.push(term);
                            }
                        }
                    }
                }
            }
            if (isSink)
            {
                // printf("is sink\n");
                std::vector<size_t> pathInst;
                size_t hops = 0;
                int lastPart = -1;
                for (size_t term : path)
                {
                    size_t inst_id = _fv.getFlatTerminalFromID(term).getFlatInstance().getID();
                    if (lastPart != -1)
                    {
                        if (_partLeaves[inst_id] != lastPart)
                        {
                            hops++;
                            lastPart = _partLeaves[inst_id];
                        }
                    }
                    else
                    {
                        lastPart = _partLeaves[inst_id];
                    }
                }
                size_t hopsTemp = 0;
                lastPart = -1;
                for (size_t term : path)
                {
                    size_t inst_id = _fv.getFlatTerminalFromID(term).getFlatInstance().getID();
                    if (_fv.getFlatTerminalFromID(term).getSnlTerm()->getDirection() ==
                        SNLTerm::Direction::DirectionEnum::Output)
                    {
                        _term2paths[term].emplace_back(PathExtractor::SubPath(hops - hopsTemp, _paths.size(), pathInst.size() - 1));
                    }
                    if (lastPart != -1)
                    {
                        if (_partLeaves[inst_id] != lastPart)
                        {
                            hopsTemp++;
                            lastPart = _partLeaves[inst_id];
                        }
                    }
                    else
                    {
                        lastPart = _partLeaves[inst_id];
                    }
                    if (pathInst.empty() || inst_id != pathInst.back())
                    {
                        pathInst.push_back(inst_id);
                        // printf("2-- %lu(%lu-%lu) %lu %lu\n", hops - hopsTemp, hops, hopsTemp, _paths.size(), pathInst.size() - 1);

                        // printf("%lu %lu\n", _paths.size(), pathInst.size() - 1);
                    }
                }

                _paths.emplace_back(std::pair<std::vector<size_t>, SubPath>(pathInst, SubPath(std::numeric_limits<size_t>::max(),
                                                                                              std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max())));
                // printf("%lu\n", _paths.size());
                // printf("%lu\n", _paths.size());
                if (stack.empty())
                {
                    break;
                }
                if (path.empty())
                {
                    continue;
                }
                // printf("stack: %lu\n", stack.size());
                // printf("path: %lu\n", path.size());
                path.pop_back(); // To remove the current input that was inserted
                if (_fv.getFlatTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    path.pop_back(); // To remove the parent ouptput
                    assert(_fv.getFlatTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                                                _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID())
                    {
                        // printf("path: %lu\n", path.size());
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                        path.pop_back();
                        if (path.empty())
                        {
                            break;
                        }
                    }
                    if (_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() !=
                        _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID())
                    {
                        // printf("%lu %lu\n", _fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID(),
                        //_fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    }
                    assert(_fv.getFlatTerminalFromID(stack.top()).getFlatInstance().getID() ==
                           _fv.getFlatTerminalFromID(path.back()).getFlatInstance().getID());
                    // printf("C\n");
                    continue;
                }
                while (!path.empty() && (path.back() !=
                                         (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
                    // printf("path: %lu\n", path.size());
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                    path.pop_back();
                    if (path.empty())
                    {
                        break;
                    }
                }
                // printf("D\n");
                assert(path.back() ==
                       (*(_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                /*if (path.empty()) {
                    continue;
                }
                path.pop_back();*/
            }
            // printf("E\n");
        }
        printf("%lu\n", path.size());
        // assert(path.size() > 0);
    }
    size_t notVisited = 0;
    size_t isVisited = 0;
    size_t nc = 0;
    size_t termI = 0;
    size_t nleaf = 0;
    size_t stubs = 0;
    for (bool term : visited)
    {
        if (!_fv.getFlatTerminalFromID(termI).getFlatInstance().isLeaf())
        {
            ++nleaf;
            ++termI;
            continue;
        }
        if (term)
            ++isVisited;
        if (_fv.getFlatTerminalFromID(termI).getSnlTerm()->getNet() == nullptr)
            ++nc;
        else if (_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(termI).getIsoID()).getDrivers().size() == 0)
        {
            ++stubs;
        }
        else if (_fv.getFlatIsoDB().getIsoFromIsoIDconst(_fv.getFlatTerminalFromID(termI).getIsoID()).getReaders().size() == 0)
        {
            ++stubs;
        }
        else if (!term)
        {
            printf("%s\n", _fv.getFlatTerminalFromID(termI).getSnlTerm()->getString().c_str());
            printf("%s\n", _fv.getFlatTerminalFromID(termI).getFlatInstance().getSNLInstance()->getString().c_str());
            printf("%lu\n", _fv.getFlatTerminalFromID(termI).getFlatInstance().getID());
            ++notVisited;
        }
        ++termI;
    }
    printf("not visited %lu is visited %lu nc %lu nleaf %lu stubs %lu\n", notVisited, isVisited, nc, nleaf, stubs);
}

void PathExtractor::printHopHistogram()
{
    std::map<size_t, size_t> histogram;
    for (const auto &path : _paths)
    {
        int lastPart = -1;
        size_t hops = 0;
        for (size_t inst : path.first)
        {
            // printf("%lu %lu - %lu\n", inst, lastPart, _partLeaves[inst] );
            if (lastPart != -1)
            {
                if (_partLeaves[inst] != lastPart)
                {
                    hops++;
                    lastPart = _partLeaves[inst];
                }
            }
            else
            {
                lastPart = _partLeaves[inst];
            }
        }
        auto pathToProcess = path;
        while (pathToProcess.second._index != std::numeric_limits<size_t>::max())
        {
            for (size_t i = pathToProcess.second._index; i < _paths[pathToProcess.second._path].first.size() - 1; ++i)
            {
                size_t inst = _paths[pathToProcess.second._path].first[i];
                if (lastPart != -1)
                {
                    if (_partLeaves[inst] != lastPart)
                    {
                        hops++;
                        lastPart = _partLeaves[inst];
                    }
                }
                else
                {
                    lastPart = _partLeaves[inst];
                }
            }
            pathToProcess = _paths[path.second._path];
        }

        histogram[hops]++;
    }
    for (const auto &entry : histogram)
    {
        printf("%lu : %lu\n", entry.first, entry.second);
    }
}