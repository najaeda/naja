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
using namespace naja::DNL;

#define DEBUG_PRINTS false

DNLInstance::DNLInstance(DNL &fv) : _dnl(fv)
{
}
DNLInstance::DNLInstance(const SNLInstance *instance, DNLID id, DNLID parent, DNL &fv) : _instance(instance), _id(id), _parent(parent), _dnl(fv)
{
    // postProcess();
}

void DNLInstance::display() const
{
    printf("fi %lu %s\n", getID(), getSNLInstance()->getString().c_str());
    for (DNLID term = getTermIndexes().first;
         term < getTermIndexes().second; term++)
    {
        printf("- ft %lu %d %s\n", term,
               (int)_dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection(), _dnl.getDNLTerminalFromID(term).getSnlTerm()->getString().c_str());
    }
}

const DNLInstance &DNLInstance::getParentInstance() const { return _dnl.getDNLInstanceFromID(_parent); };
DNLID DNLInstance::getID() const { return _id; }
DNLID DNLInstance::getParentID() const { return _parent; }
// const std::vector<const SNLInstance*>& getFullInstanceBranch() const { return _fullInstanceBranch; }
const SNLInstance *DNLInstance::getSNLInstance() const { return _instance; }
void DNLInstance::setTermsIndexes(const std::pair<DNLID, DNLID> &termsIndexes) { _termsIndexes = termsIndexes; }
void DNLInstance::setChildrenIndexes(const std::pair<DNLID, DNLID> &childrenIndexes) { _childrenIndexes = childrenIndexes; }
const DNLInstance &DNLInstance::getChildInstance(const SNLInstance *snlInst) const
{
#ifdef DEBUG_PRINTS
    printf("%lu %lu %lu %d\n", _id, _childrenIndexes.first, _childrenIndexes.second, (int)isTop());
#endif
    for (DNLID child = _childrenIndexes.first + 1; child <= _childrenIndexes.second; child++)
    {
#ifdef DEBUG_PRINTS
        printf("%p %p\n", snlInst, _dnl.getDNLInstanceFromID(child).getSNLInstance());
#endif
        if (_dnl.getDNLInstanceFromID(child).getSNLInstance() == snlInst)
        {
            return _dnl.getDNLInstanceFromID(child);
        }
    }
#ifdef DEBUG_PRINTS
    printf("null inst\n");
#endif
    return _dnl.getDNLNullInstance();
}
const DNLTerminal &DNLInstance::getTerminal(const SNLInstTerm *snlTerm) const
{
    for (DNLID term = _termsIndexes.first; term < _termsIndexes.second; term++)
    {
        if (_dnl.getDNLTerminalFromID(term).getSnlTerm() == snlTerm)
        {
            return _dnl.getDNLTerminalFromID(term);
        }
    }
#ifdef DEBUG_PRINTS
    printf("null term\n");
#endif
    return _dnl.getDNLNullTerminal();
}
const DNLTerminal &DNLInstance::getTerminal(const SNLBitTerm *snlTerm) const
{
    for (DNLID term = _termsIndexes.first; term < _termsIndexes.second; term++)
    {
        if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getTerm() == snlTerm)
        {
#ifdef DEBUG_PRINTS
            printf("return %lu %s %s\n", term, _dnl.getDNLTerminalFromID(term).getSnlTerm()->getString().c_str(),
                   _dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection().getString().c_str());
#endif
            return _dnl.getDNLTerminalFromID(term);
        }
    }
    assert(false);
#ifdef DEBUG_PRINTS
    printf("return null terminal\n");
#endif
    return _dnl.getDNLNullTerminal();
}

// DNLLeafInstance instance

// DNLHierInstsance -> minimal entry for parents

DNLTerminal::DNLTerminal(DNL &fv, DNLID id) : _dnl(fv), _id(id){};
DNLTerminal::DNLTerminal(DNLID DNLInstID, SNLInstTerm *terminal, DNLID id, DNL &fv) : _DNLInstID(DNLInstID), _terminal(terminal), _id(id), _dnl(fv){};
DNLID DNLTerminal::getID() const { return _id; }
SNLInstTerm *DNLTerminal::getSnlTerm() const { return _terminal; }
const DNLInstance &DNLTerminal::getDNLInstance() const
{
    return _dnl.getDNLInstanceFromID(_DNLInstID);
}

void DNLTerminal::setIsoID(DNLID isoID) { _dnl.setIsoIdforTermId(isoID, _id); }
DNLID DNLTerminal::getIsoID() const { return _dnl.getIsoIdfromTermId(_id); }

DNLIso::DNLIso(DNLID id, const DNL &fv) : _id(id), _dnl(fv){};
void DNLIso::addDriver(DNLID driver) { _drivers.push_back(driver); }
void DNLIso::addReader(DNLID reader) { _readers.push_back(reader); }
void DNLIso::display(std::ostream &stream) const
{
    for (auto &driver : _drivers)
    {
        stream << "driver instance" << _dnl.getDNLTerminalFromID(driver).getSnlTerm()->getInstance()->getName().getString() << std::endl
               << _dnl.getDNLTerminalFromID(driver).getSnlTerm()->getInstance()->getDescription() << std::endl;
        ;
        stream << "driver " << _dnl.getDNLTerminalFromID(driver).getSnlTerm()->getString() << std::endl;
        stream << "driver " << _dnl.getDNLTerminalFromID(driver).getSnlTerm()->getDescription() << std::endl;
    }
    for (auto &reader : _readers)
    {
        stream << "reader instance" << _dnl.getDNLTerminalFromID(reader).getSnlTerm()->getInstance()->getName().getString() << std::endl;
        ;
        stream << "reader" << _dnl.getDNLTerminalFromID(reader).getSnlTerm()->getString() << std::endl;
        ;
    }
}

DNLIsoDB::DNLIsoDB(const DNL &fv) : _dnl(fv) {}

DNLIso &DNLIsoDB::addIso()
{
    _isos.push_back(DNLIso(_isos.size(), _dnl));
    return _isos.back();
}

DNLIsoDBBuilder::DNLIsoDBBuilder(DNLIsoDB &db, DNL &fv) : _db(db), _dnl(fv) { _visited.resize(_dnl.getDNLTerms().size(), false); }

void DNLIsoDBBuilder::process()
{
    // iterate on all leaf drivers
    std::vector<DNLID> tasks;
    for (DNLID leaf : _dnl.getLeaves())
    {
        for (DNLID term = _dnl.getDNLInstanceFromID(leaf).getTermIndexes().first;
             term < _dnl.getDNLInstanceFromID(leaf).getTermIndexes().second; term++)
        {
            if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                DNLIso &DNLIso = addIsoToDB();

                DNLIso.addDriver(term);
                tasks.push_back(term);
                _dnl.getDNLTerminalFromID(term).setIsoID(DNLIso.getIsoID());
            }
        }
    }
    if (!getenv("NON_MT_dnl"))
    {
        printf("MT\n");
        tbb::task_scheduler_init init(tbb::task_scheduler_init::default_num_threads()); // Explicit number of threads
        tbb::parallel_for(tbb::blocked_range<DNLID>(0, tasks.size()),
                          [&](const tbb::blocked_range<DNLID> &r)
                          {
                              for (DNLID i = r.begin(); i < r.end(); ++i)
                              {
                                  treatDriver(_dnl.getDNLTerminalFromID(tasks[i]));
                              }
                          });
    }
    else
    {
        printf("Non MT\n");
        for (auto task : tasks)
        {
            treatDriver(_dnl.getDNLTerminalFromID(task));
        }
    }

    printf("num fi %lu\n", _dnl.getDNLInstances().size());
    printf("num ft %lu\n", _dnl.getDNLTerms().size());
    printf("num leaves %lu\n", _dnl.getLeaves().size());
    printf("num isos %lu\n", _db.getNumIsos());
}

void DNLIsoDB::display() const
{
    printf("----------ISODB - BEGIN----------\n");
    for (const DNLIso &iso : _isos)
    {
        printf("----------new iso----------\n");
        iso.display();
    }
    printf("----------ISODB - END----------\n");
}

void DNLIsoDBBuilder::treatDriver(const DNLTerminal &term)
{
#ifdef DEBUG_PRINTS
    printf("leaf -%s\n", term.getSnlTerm()->getInstance()->getName().getString().c_str());
    printf("#############################################################\n");
    printf("treating %lu %p %s %s\n", term.getID(), term.getSnlTerm(),
           term.getSnlTerm()->getString().c_str(), term.getSnlTerm()->getDirection().getString().c_str());
#endif
    assert(term.getSnlTerm()->getInstance()->getModel()->getInstances().empty());
    std::stack<DNLID> stack;
    DNLIso &DNLIso = _db.getIsoFromIsoID(term.getIsoID());
    // do DFS on the driver
    stack.push(term.getID());
    // Start traversing from driver
    while (!stack.empty())
    {
        DNLID id = stack.top();
        stack.pop();
        const DNLTerminal &fterm = _dnl.getDNLTerminalFromID(id);
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
        if (fterm.getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output && fterm.getDNLInstance().isTop())
        {
#ifdef DEBUG_PRINTS
            printf("--is top output so adding as reader\n");
#endif
            DNLIso.addReader(fterm.getID());
            _dnl.getDNLTerminalFromID(fterm.getID()).setIsoID(DNLIso.getIsoID());
            continue;
        }
        _visited[id] = true;
        SNLBitNet *snlNet = fterm.getSnlTerm()->getNet();
        DNLIso.addNet(snlNet);
        const DNLInstance *DNLParent = &fterm.getDNLInstance().getParentInstance();
        // Get snl bit net connected to the snl term object of the DNL terminal
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
            DNLParent = &fterm.getDNLInstance();
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
            const DNLTerminal &ftermNew = goDown ? fterm.getDNLInstance().getChildInstance(termSnlInst).getTerminal(instTerm) : fterm.getDNLInstance().getParentInstance().getChildInstance(termSnlInst).getTerminal(instTerm);
#ifdef DEBUG_PRINTS
            printf("--visiting snl  it %lu %p %s %s\n", ftermNew.getID(), termSnlInst, instTerm->getString().c_str(), instTerm->getDirection().getString().c_str());
            printf("--visiting DNL it %lu %p %s %s\n", ftermNew.getID(), ftermNew.getSnlTerm(), ftermNew.getSnlTerm()->getString().c_str(), ftermNew.getSnlTerm()->getDirection().getString().c_str());
#endif
            if (termSnlInst->getModel()->getInstances().empty())
            {
                if (ftermNew.getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
                {
                    // if (ftermNew.getID() == term.getID()) continue;
                    if (term.getID() != ftermNew.getID())
                    {
                        assert(false);
                        DNLIso.addDriver(ftermNew.getID());
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
                    DNLIso.addReader(ftermNew.getID());
                    _dnl.getDNLTerminalFromID(ftermNew.getID()).setIsoID(DNLIso.getIsoID());
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
            DNLParent->display();
            DNLParent->getParentInstance().display();
            printf("--visiting bt %lu %p %s %s\n", DNLParent->getTerminal(bitTerm).getID(), bitTerm, bitTerm->getString().c_str(), bitTerm->getDirection().getString().c_str());
#endif
            const DNLTerminal &ftermNew = DNLParent->getTerminal(bitTerm);
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

DNL::DNL(const SNLDesign *top) : _top(top), _fidb(*this)
{
}

void DNL::dumpDotFile() const
{
    std::ofstream myfile;
    myfile.open("./netlist.dat");
    std::stack<DNLID> stack;
    stack.push(0);
    myfile << "digraph " << _top->getName().getString() << " {\n rankdir=LR\n";
    /*for (SNLInstance *inst : _top->getInstances())
    {
        dumpDotFileRec(inst->getModel(), myfile);
    }*/
    DNLID i = 0;
    dumpDotFileRec(0, myfile, i);
    myfile << "}";
    myfile.close();
}

void DNL::dumpDotFileRec(DNLID inst, std::ofstream &myfile, DNLID &i) const
{
    if (getDNLInstanceFromID(inst).getChildren().first == getDNLInstanceFromID(inst).getChildren().second)
    {
        // myfile << getDNLInstanceFromID(inst).getSNLInstance()->getName().getString() << std::endl;;
        myfile << "leaf" << inst << " [shape=record, label=\"" << getDNLInstanceFromID(inst).getSNLInstance()->getName().getString() << "\"];" << std::endl;
        return;
    }

    if (inst != 1)
    {
        myfile << "subgraph cluster_" << i << " {" << std::endl;
        myfile << "label = \""
               << getDNLInstanceFromID(inst).getSNLInstance()->getName().getString() << "\";" << std::endl;
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
    printf("here %lu %lu\n", getDNLInstanceFromID(inst).getChildren().first + 1,
           getDNLInstanceFromID(inst).getChildren().second);
    for (DNLID child = getDNLInstanceFromID(inst).getChildren().first + 1;
         child <= getDNLInstanceFromID(inst).getChildren().second; child++)
    {
        printf("here\n");
        dumpDotFileRec(child, myfile, i);
    }
    myfile
        << "}" << std::endl;
}

void DNL::dumpDNLDotFile(bool keepHierInfo) const
{
    std::ofstream myfile;

    DNLID i = 0;
    if (keepHierInfo)
    {

        myfile.open(std::string("./") + _top->getName().getString() + std::string(".dat"));
        myfile << "digraph DNLNetlist {" << std::endl;
        dumpDotFileRec(1, myfile, i);
    }
    else
    {
        myfile.open("./DNLNetlist.dat");
        myfile << "digraph DNLNetlist {" << std::endl;
    }

    /*for (DNLID leaf : _leaves) {
        myfile << "leaf" << leaf << std::endl;
    }*/
    myfile << std::endl;
    // for (DNLID isoId : _fidb.getNumIsos()) {
    for (DNLID isoId = 0; isoId < _fidb.getNumIsos(); isoId++)
    {
        const DNLIso &iso = _fidb.getIsoFromIsoIDconst(isoId);
        for (DNLID driver : iso.getDrivers())
        {
            for (DNLID reader : iso.getReaders())
            {
                myfile << "leaf" << getDNLTerminalFromID(driver).getDNLInstance().getID() << " -> "
                       << "leaf" << getDNLTerminalFromID(reader).getDNLInstance().getID() << std::endl;
                // myfile <<  getDNLInstanceFromID(driver).getSNLInstance()->getName().getString() << " -> "
                //<<  getDNLInstanceFromID(reader).getSNLInstance()->getName().getString()<< std::endl;
            }
        }
    }
    myfile << "}";
}

void DNL::display() const
{
    printf("---------FV--------\n");
    for (const DNLInstance &inst : _DNLInstances)
    {
        if (inst.getSNLInstance() == nullptr)
            continue; // top
        printf("fi %lu %s\n", inst.getID(), inst.getSNLInstance()->getString().c_str());
        for (DNLID term = inst.getTermIndexes().first;
             term < inst.getTermIndexes().second; term++)
        {
            printf("- ft %lu %d %s\n", term, (int)getDNLTerminalFromID(term).getSnlTerm()->getDirection(), getDNLTerminalFromID(term).getSnlTerm()->getString().c_str());
        }
    }
    _fidb.display();
}

void DNL::process()
{
    std::vector<DNLID> stack;
    _DNLInstances.push_back(DNLInstance(nullptr, _DNLInstances.size() + 1, 0, *this));
    assert(_DNLInstances.back().getID() == _DNLInstances.size());
    DNLID parentId = _DNLInstances.back().getID();
    // DNLInstance& DNLParent = getNonConstDNLInstanceFromID(_DNLInstances.back().getID());
    std::pair<DNLID, DNLID> childrenIndexes;
    childrenIndexes.first = _DNLInstances.back().getID();
    for (auto inst : _top->getInstances())
    {
        _DNLInstances.push_back(DNLInstance(inst, _DNLInstances.size() + 1, parentId, *this));
        stack.push_back(_DNLInstances.back().getID());
        std::pair<DNLID, DNLID> termIndexes;
        termIndexes.first = _DNLTerms.size();
        if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() || inst->getModel()->getInstances().empty())
        {
            _leaves.push_back(_DNLInstances.back().getID());
        }
        for (auto term : inst->getInstTerms())
        {
            _DNLTerms.push_back(DNLTerminal(_DNLInstances.back().getID(), term, _DNLTerms.size(), *this));
        }
        termIndexes.second = _DNLTerms.size();
        _DNLInstances.back().setTermsIndexes(termIndexes);
    }
    childrenIndexes.second = _DNLInstances.back().getID();
    getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
    while (!stack.empty())
    {
#ifdef DEBUG_PRINTS
        printf("check %p\n", _DNLInstances[0].getSNLInstance());
#endif
        const SNLInstance *parent = getNonConstDNLInstanceFromID((stack.back())).getSNLInstance();
        DNLID parentId = getNonConstDNLInstanceFromID((stack.back())).getID();
        stack.pop_back();
        std::pair<DNLID, DNLID> childrenIndexes;
        childrenIndexes.first = _DNLInstances.back().getID();
        for (auto inst : parent->getModel()->getInstances())
        {
#ifdef DEBUG_PRINTS
            printf("push %p\n", inst);
            printf("push -%s\n", inst->getName().getString().c_str());
#endif
            // if (!inst) continue;
            _DNLInstances.push_back(DNLInstance(inst, _DNLInstances.size() + 1, parentId, *this));
#ifdef DEBUG_PRINTS
            printf("-%s\n", inst->getName().getString().c_str());
#endif
            stack.push_back(_DNLInstances.back().getID());
            // DNLParent->addChild(_DNLInstances.back().getID());
            if (inst->getModel()->isBlackBox() || inst->getModel()->isPrimitive() || inst->getModel()->getInstances().empty())
            {
                _leaves.push_back(_DNLInstances.back().getID());
#ifdef DEBUG_PRINTS
                printf("leaf %lu -%s\n", _DNLInstances.back().getID(),
                       inst->getName().getString().c_str());
#endif
            }
            std::pair<DNLID, DNLID> termIndexes;
            termIndexes.first = _DNLTerms.size();
            for (auto term : inst->getInstTerms())
            {
                _DNLTerms.push_back(DNLTerminal(_DNLInstances.back().getID(), term, _DNLTerms.size(), *this));
#ifdef DEBUG_PRINTS
                printf("term %lu -%s\n", _DNLTerms.back().getID(), term->getString().c_str());
#endif
            }
            termIndexes.second = _DNLTerms.size();
            _DNLInstances.back().setTermsIndexes(termIndexes);
        }
        childrenIndexes.second = _DNLInstances.back().getID();
        getNonConstDNLInstanceFromID(parentId).setChildrenIndexes(childrenIndexes);
    }
    _DNLTerms.push_back(DNLTerminal(*this, _DNLTerms.size()));
    _DNLInstances.push_back(DNLInstance(*this));
    DNLIsoDBBuilder fidbb(_fidb, *this);
    fidbb.process();
}

void DNL::dumpPartitionedDNLDotFile(const std::vector<std::vector<DNLID>> &partitions) const
{
    std::ofstream myfile;

    DNLID i = 0;
    myfile.open("./pfn.dat");
    myfile << "digraph DNLNetlist {" << std::endl;
    DNLID part = 0;
    for (const std::vector<DNLID> &partition : partitions)
    {
        myfile << "subgraph cluster_" << part << " {" << std::endl;
        myfile << "label = \""
               << "part " << part << "\";" << std::endl;
        ;
        for (DNLID leaf : partition)
        {
            myfile << "leaf" << leaf << std::endl;
        }
        myfile << "}" << std::endl;
        part++;
    }
    /*for (DNLID leaf : _leaves) {
        myfile << "leaf" << leaf << std::endl;
    }*/
    myfile << std::endl;
    // for (DNLID isoId : _fidb.getNumIsos()) {
    for (DNLID isoId = 0; isoId < _fidb.getNumIsos(); isoId++)
    {
        const DNLIso &iso = _fidb.getIsoFromIsoIDconst(isoId);
        for (DNLID driver : iso.getDrivers())
        {
            for (DNLID reader : iso.getReaders())
            {
                myfile << "leaf" << getDNLTerminalFromID(driver).getDNLInstance().getID() << " -> "
                       << "leaf" << getDNLTerminalFromID(reader).getDNLInstance().getID() << std::endl;
            }
        }
    }
    myfile << "}";
}

std::vector<DNLID> DNL::getLeavesUnder(DNLID parent) const
{
    std::vector<DNLID> leaves;
    std::stack<DNLID> toProcess;
    toProcess.push(parent);
    while (!toProcess.empty())
    {

        const DNLInstance &inst = getDNLInstanceFromID(toProcess.top());
        if (inst.isLeaf())
            leaves.push_back(toProcess.top());
        toProcess.pop();
        for (DNLID child = inst.getChildren().first + 1;
             child <= inst.getChildren().second; child++)
        {
            toProcess.push(child);
        }
    }
    return leaves;
}

bool DNL::isInstanceChild(DNLID parent, DNLID child) const
{
    DNLID inst = child;
    if (parent == 1)
    {
        return true;
    }
    while (getDNLInstanceFromID(child).getParentID() != 1)
    {
        if (parent == inst)
        {
            return true;
        }
        inst = getDNLInstanceFromID(child).getParentID();
    }
    return false;
}

void PathExtractor::cachePaths()
{
    std::vector<DNLID> sources;
    const std::vector<DNLID, tbb::scalable_allocator<DNLID>> &leaves = _dnl.getLeaves();
    std::vector<bool> visited(_dnl.getNBterms(), false);
    DNLID criticalDelay = 0;
    printf("leaves number: %lu\n", leaves.size());
    for (DNLID leaf : leaves)
    {
        bool source = true;
        for (DNLID term = _dnl.getDNLInstanceFromID(leaf).getTermIndexes().first;
             term < _dnl.getDNLInstanceFromID(leaf).getTermIndexes().second; term++)
        {
            if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection() != SNLTerm::Direction::DirectionEnum::Output && _dnl.getDNLTerminalFromID(term).getSnlTerm()->getNet() != nullptr)
            {
                source = false;
            }
        }
        if (source)
        {
            for (DNLID term = _dnl.getDNLInstanceFromID(leaf).getTermIndexes().first;
                 term < _dnl.getDNLInstanceFromID(leaf).getTermIndexes().second; term++)
            {
                sources.push_back(term);
            }
        }
    }
    for (DNLID source : sources)
    {
        std::stack<DNLID> stack;
        std::vector<DNLID> path;
        stack.push(source);
        std::vector<bool> visitedLoops(_dnl.getNBterms(), false);
        // std::vector<bool> visited(_dnl.getNBterms(), false);
        while (!stack.empty())
        {
            /*printf("--------------\n");
            for (DNLID p : path) {
                printf("---%s\n",  _dnl.getDNLTerminalFromID(p).getSnlTerm()->getString().c_str());
            }
            printf("path %lu\n", path.size());*/
            if (_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().size() > 1)
            {

                visited[stack.top()] = true;
                stack.pop();
                continue; // MD
            }
            if (_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().size() == 0)
            {

                visited[stack.top()] = true;
                stack.pop();
                continue; // Stub
            }
            assert(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().size() == 1);
            DNLID toProcess = stack.top();
            // visited[toProcess] = true;
            if (toProcess != source)
                assert(!path.empty());
#ifdef DEBUG_PRINTS
            printf("%s\n", _dnl.getDNLTerminalFromID(toProcess).getSnlTerm()->getString().c_str());
            printf("%lu\n", toProcess);
#endif
            if (!path.empty())
            {
#ifdef DEBUG_PRINTS
                printf("%s\n", _dnl.getDNLTerminalFromID(toProcess).getSnlTerm()->getString().c_str());
                printf("%lu\n", toProcess);
                printf("driver %lu\n", (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));

                printf("path: %lu %s\n", path.back(), _dnl.getDNLTerminalFromID(path.back()).getSnlTerm()->getString().c_str());
                printf("iso: %lu %lu\n", _dnl.getDNLTerminalFromID(path.back()).getIsoID(), _dnl.getDNLTerminalFromID(stack.top()).getIsoID());
                printf("inst: %lu %lu\n", _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID(),
                       _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
#endif
                assert(_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() ==
                           _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID() ||
                       (path.back() ==
                        (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))));
            }

            stack.pop();
            if (visitedLoops[toProcess] &&
                _dnl.getDNLTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Input)
            { // Loop
                if (_dnl.getDNLTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    path.pop_back();
                    assert(_dnl.getDNLTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                                                _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID())
                    {
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
                    /*if (_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                        _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID()) {
                        //printf("%lu %lu\n", _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID(),
                        //_dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    }*/
                    assert(_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() ==
                           _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    continue;
                }
                // path.pop_back();//To remove the previous input
                while (!path.empty() && (path.back() !=
                                         (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
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
                       (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                continue;
            }

            if (visited[toProcess] &&
                _dnl.getDNLTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                DNLID hopsInit = 0;
                DNLID lastPart = DNLID_MAX;
                for (DNLID term : path)
                {
                    DNLID inst_id = _dnl.getDNLTerminalFromID(term).getDNLInstance().getID();
                    if (lastPart != DNLID_MAX)
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
                    std::vector<DNLID> pathInst;
                    DNLID hops = hopsInit;
                    lastPart = DNLID_MAX;
                    hops += toCache._delay;
                    DNLID hopTemp = 0;

                    for (DNLID term : path)
                    {
                        DNLID inst_id = _dnl.getDNLTerminalFromID(term).getDNLInstance().getID();
                        if (lastPart != DNLID_MAX)
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
                            _term2paths[_dnl.getDNLTerminalFromID(term).getDNLInstance().getID()].emplace_back(
                                PathExtractor::SubPath(hops - hopTemp, _paths.size(), pathInst.size() - 1));
                        }
                    }
                    _paths.emplace_back(std::pair<std::vector<DNLID>, SubPath>(pathInst, toCache));
                }
                if (stack.empty())
                {
                    break;
                }
                if (path.empty())
                {
                    continue;
                }
                // path.pop_back();//To remove the previous input
                if (_dnl.getDNLTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    // path.pop_back();
                    assert(_dnl.getDNLTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                                                _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID())
                    {
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
                    /*if (_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                        _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID()) {
                        //printf("%lu %lu\n", _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID(),
                        //_dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    }*/
                    assert(_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() ==
                           _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    assert(visited[toProcess]);
                    continue;
                }
                path.pop_back(); // To remove the previous input
                while (!path.empty() && (path.back() !=
                                         (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
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
                assert(visited[toProcess]);
                assert(path.back() ==
                       (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                continue;
            }
            visited[toProcess] = true;
            visitedLoops[toProcess] = true;
            path.push_back(toProcess);
            bool isSink = false;
            if (_dnl.getDNLTerminalFromID(toProcess).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
            {
                const DNLIso &iso = _dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(toProcess).getIsoID());
                for (DNLID reader : iso.getReaders())
                {
                    isSink = false;
                    stack.push(reader);
                    assert(_dnl.getDNLTerminalFromID(reader).getIsoID() == _dnl.getDNLTerminalFromID(toProcess).getIsoID());
                }
            }
            else
            {
                isSink = true;
                if (path.size() <= 20 || true)
                {
                    DNLID inst_id = _dnl.getDNLTerminalFromID(toProcess).getDNLInstance().getID();
                    for (DNLID term = _dnl.getDNLInstanceFromID(inst_id).getTermIndexes().first;
                         term < _dnl.getDNLInstanceFromID(inst_id).getTermIndexes().second; term++)
                    {
                        if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection() == SNLTerm::Direction::DirectionEnum::Output)
                        {

                            assert(_dnl.getDNLTerminalFromID(term).getDNLInstance().getID() ==
                                   _dnl.getDNLTerminalFromID(toProcess).getDNLInstance().getID());
                            if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getNet() != nullptr)
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
#ifdef DEBUG_PRINTS
                printf("is sink\n");
#endif
                std::vector<DNLID> pathInst;
                DNLID hops = 0;
                int lastPart = DNLID_MAX;
                for (DNLID term : path)
                {
                    DNLID inst_id = _dnl.getDNLTerminalFromID(term).getDNLInstance().getID();
                    if (lastPart != DNLID_MAX)
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
                DNLID hopsTemp = 0;
                lastPart = DNLID_MAX;
                for (DNLID term : path)
                {
                    DNLID inst_id = _dnl.getDNLTerminalFromID(term).getDNLInstance().getID();
                    if (_dnl.getDNLTerminalFromID(term).getSnlTerm()->getDirection() ==
                        SNLTerm::Direction::DirectionEnum::Output)
                    {
                        _term2paths[term].emplace_back(PathExtractor::SubPath(hops - hopsTemp, _paths.size(), pathInst.size() - 1));
                    }
                    if (lastPart != DNLID_MAX)
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
                    }
                }

                _paths.emplace_back(std::pair<std::vector<DNLID>, SubPath>(pathInst, SubPath(std::numeric_limits<DNLID>::max(),
                                                                                             std::numeric_limits<DNLID>::max(), std::numeric_limits<DNLID>::max())));

                if (stack.empty())
                {
                    break;
                }
                if (path.empty())
                {
                    continue;
                }
                path.pop_back(); // To remove the current input that was inserted
                if (_dnl.getDNLTerminalFromID(stack.top()).getSnlTerm()->getDirection() ==
                    SNLTerm::Direction::DirectionEnum::Output)
                {
                    path.pop_back(); // To remove the parent ouptput
                    assert(_dnl.getDNLTerminalFromID(path.back()).getSnlTerm()->getDirection() ==
                           SNLTerm::Direction::DirectionEnum::Input);
                    while (!path.empty() && _dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                                                _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID())
                    {
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
                    if (_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() !=
                        _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID())
                    {
                        //_dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    }
                    assert(_dnl.getDNLTerminalFromID(stack.top()).getDNLInstance().getID() ==
                           _dnl.getDNLTerminalFromID(path.back()).getDNLInstance().getID());
                    continue;
                }
                while (!path.empty() && (path.back() !=
                                         (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin()))))
                {
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
                       (*(_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(stack.top()).getIsoID()).getDrivers().begin())));
                /*if (path.empty()) {
                    continue;
                }
                path.pop_back();*/
            }
        }
#ifdef DEBUG_PRINTS
        printf("%lu\n", path.size());
#endif
        // assert(path.size() > 0);
    }
    DNLID notVisited = 0;
    DNLID isVisited = 0;
    DNLID nc = 0;
    DNLID termI = 0;
    DNLID nleaf = 0;
    DNLID stubs = 0;
    for (bool term : visited)
    {
        if (!_dnl.getDNLTerminalFromID(termI).getDNLInstance().isLeaf())
        {
            ++nleaf;
            ++termI;
            continue;
        }
        if (term)
            ++isVisited;
        if (_dnl.getDNLTerminalFromID(termI).getSnlTerm()->getNet() == nullptr)
            ++nc;
        else if (_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(termI).getIsoID()).getDrivers().size() == 0)
        {
            ++stubs;
        }
        else if (_dnl.getDNLIsoDB().getIsoFromIsoIDconst(_dnl.getDNLTerminalFromID(termI).getIsoID()).getReaders().size() == 0)
        {
            ++stubs;
        }
        else if (!term)
        {
#ifdef DEBUG_PRINTS
            printf("%s\n", _dnl.getDNLTerminalFromID(termI).getSnlTerm()->getString().c_str());
            printf("%s\n", _dnl.getDNLTerminalFromID(termI).getDNLInstance().getSNLInstance()->getString().c_str());
            printf("%lu\n", _dnl.getDNLTerminalFromID(termI).getDNLInstance().getID());
#endif
            ++notVisited;
        }
        ++termI;
    }
#ifdef DEBUG_PRINTS
    printf("not visited %lu is visited %lu nc %lu nleaf %lu stubs %lu\n", notVisited, isVisited, nc, nleaf, stubs);
#endif
}

void PathExtractor::printHopHistogram()
{
    std::map<DNLID, DNLID> histogram;
    for (const auto &path : _paths)
    {
        int lastPart = DNLID_MAX;
        DNLID hops = 0;
        for (DNLID inst : path.first)
        {
            if (lastPart != DNLID_MAX)
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
        while (pathToProcess.second._index != std::numeric_limits<DNLID>::max())
        {
            for (DNLID i = pathToProcess.second._index; i < _paths[pathToProcess.second._path].first.size() - 1; ++i)
            {
                DNLID inst = _paths[pathToProcess.second._path].first[i];
                if (lastPart != DNLID_MAX)
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