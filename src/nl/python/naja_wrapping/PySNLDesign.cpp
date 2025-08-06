// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesign.h"

#include "PyInterface.h"

#include "PyNLDB.h"
#include "PyNLLibrary.h"

#include "PySNLScalarTerm.h"
#include "PySNLBusTerm.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNet.h"
#include "PySNLInstance.h"
#include "PySNLParameter.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLInstTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLNets.h"
#include "PySNLScalarNets.h"
#include "PySNLBusNets.h"
#include "PySNLBitNets.h"
#include "PySNLInstances.h"
#include "PySNLParameters.h"

#include "SNLDesign.h"
#include "SNLDesignModeling.h"
#include "SNLDesignTruthTable.h"
#include "SNLTruthTable.h"
#include "SNLVRLDumper.h"

#include "NetlistGraph.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesign, function)

static PyObject* PySNLDesign_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDesign.create", &arg0, &arg1)) {
    setError("malformed SNLDesign create method");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  SNLDesign* design = nullptr;
  TRY
  if (IsPyNLLibrary(arg0)) {
    design = SNLDesign::create(PYNLLibrary_O(arg0), name);
  } else {
    setError("SNLDesign create accepts NLLibrary as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_createPrimitive(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    setError("malformed SNLDesign createPrimitive method");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  SNLDesign* design = nullptr;
  TRY
  if (IsPyNLLibrary(arg0)) {
    design = SNLDesign::create(PYNLLibrary_O(arg0), SNLDesign::Type::Primitive, name);
  } else {
    setError("SNLDesign createPrimitive accepts NLLibrary as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_clone(PySNLDesign* self, PyObject* args) {
  const char* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "|s:SNLDesign.clone", &arg0)) {
    setError("malformed SNLDesign.clone method");
    return nullptr;
  }
  NLName name;
  if (arg0) {
    name = NLName(arg0);
  }

  SNLDesign* newDesign = nullptr;
  METHOD_HEAD("SNLDesign.clone()")
  TRY
  newDesign = selfObject->clone(name);
  NLCATCH
  return PySNLDesign_Link(newDesign);
}

static PyObject* PySNLDesign_dumpVerilog(PySNLDesign* self, PyObject* args) {
  const char* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "ss:SNLDesign.dumpVerilog", &arg0, &arg1)) {
    setError("malformed SNLDesign.dumpVerilog method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.dumpVerilog()")
  TRY
  SNLVRLDumper dumper;
  dumper.setTopFileName(arg1);
  dumper.dumpDesign(selfObject, std::filesystem::path(arg0));
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_addCombinatorialArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addCombinatorialArcs", &arg0, &arg1)) {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  SNLDesignModeling::BitTerms terms0;
  SNLDesignModeling::BitTerms terms1;
  if (IsPySNLBitTerm(arg0)) {
    terms0.push_back(PYSNLBitTerm_O(arg0));
  } else if (IsPySNLBusTerm(arg0)) {
    auto bus = PYSNLBusTerm_O(arg0);
    terms0.insert(terms0.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    terms1.push_back(PYSNLBitTerm_O(arg1));
  } else if (IsPySNLBusTerm(arg1)) {
    auto bus = PYSNLBusTerm_O(arg1);
    terms1.insert(terms1.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg1)) {
    setError("malformed SNLDesign.addCombinatorialArcs method");
    return nullptr;
  }
  if (terms0.empty()) {
    for (int i=0; i<PyList_Size(arg0); ++i) {
      PyObject* object = PyList_GetItem(arg0, i);
      if (IsPySNLBitTerm(object)) {
        terms0.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms0.insert(terms0.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addCombinatorialArcs method");
        return nullptr;
      }
    }
  }
  if (terms1.empty()) {
    for (int j=0; j<PyList_Size(arg1); ++j) {
      PyObject* object = PyList_GetItem(arg1, j);
      if (IsPySNLBitTerm(object)) {
        terms1.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms1.insert(terms1.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addCombinatorialArcs method");
        return nullptr;
      }
    }
  }
  TRY
  SNLDesignModeling::addCombinatorialArcs(terms0, terms1);
  NLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_addInputsToClockArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addInputsToClockArcs", &arg0, &arg1)) {
    setError(
      "malformed SNLDesign.addInputsToClockArcs method, accepted args are: inputs, clock."
    );
    return nullptr;
  }
  SNLDesignModeling::BitTerms terms;
  SNLBitTerm* clock = nullptr; 
  if (IsPySNLBitTerm(arg0)) {
    terms.push_back(PYSNLBitTerm_O(arg0));
  } else if (IsPySNLBusTerm(arg0)) {
    auto bus = PYSNLBusTerm_O(arg0);
    terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg0)) {
    setError(
      "malformed SNLDesign.addInputsToClockArcs method, "
      "accepted args for first argument are a list of SNLBitTerm, SNLBusTerm");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    clock = PYSNLBitTerm_O(arg1);
  } else {
    setError("malformed SNLDesign.addInputsToClockArcs method");
    return nullptr;
  }
  if (terms.empty()) {
    for (int i=0; i<PyList_Size(arg0); ++i) {
      PyObject* object = PyList_GetItem(arg0, i);
      if (IsPySNLBitTerm(object)) {
        terms.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addInputsToClockArcs method");
        return nullptr;
      }
    }
  }
  SNLDesignModeling::addInputsToClockArcs(terms, clock);
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_addClockToOutputsArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addClockToOutputsArcs", &arg0, &arg1)) {
    setError(
      "malformed SNLDesign.addClockToOutputsArcs method, accepted args are: clock, outputs."
    );
    return nullptr;
  }
  SNLBitTerm* clock = nullptr; 
  SNLDesignModeling::BitTerms terms;
  if (IsPySNLBitTerm(arg0)) {
    clock = PYSNLBitTerm_O(arg0);
  } else {
    setError("malformed SNLDesign.addClockToOutputsArcs: accepted first arg is: clock");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    terms.push_back(PYSNLBitTerm_O(arg1));
  } else if (IsPySNLBusTerm(arg1)) {
    auto bus = PYSNLBusTerm_O(arg1);
    terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
  } else if (not PyList_Check(arg1)) {
    setError(
      "malformed SNLDesign.addClockToOutputsArcs method, "
      "accepted args for second argument are a list of SNLBitTerm, SNLBusTerm");
    return nullptr;
  }
  if (terms.empty()) {
    for (int i=0; i<PyList_Size(arg1); ++i) {
      PyObject* object = PyList_GetItem(arg1, i);
      if (IsPySNLBitTerm(object)) {
        terms.push_back(PYSNLBitTerm_O(object));
      } else if (IsPySNLBusTerm(object)) {
        auto bus = PYSNLBusTerm_O(object);
        terms.insert(terms.begin(), bus->getBits().begin(), bus->getBits().end());
      } else {
        setError("malformed SNLDesign.addInputsToClockArcs method");
        return nullptr;
      }
    }
  } 
  SNLDesignModeling::addClockToOutputsArcs(clock, terms);
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_setTruthTable(PySNLDesign* self, PyObject* args) {
  uint64_t tt = 0;
  if (not PyArg_ParseTuple(args, "K:SNLDesign.setTruthTable", &tt)) {
    setError("malformed SNLDesign.setTruthTable method");
    return nullptr;
  }
  METHOD_HEAD("SNLDesign.setTruthTable()")
  auto filter = [](const SNLTerm* term) { return term->getDirection() == SNLTerm::Direction::Input; };
  size_t size = selfObject->getBitTerms().getSubCollection(filter).size();
  SNLTruthTable truthTable(size, tt);
  SNLDesignTruthTable::setTruthTable(selfObject, truthTable);
  Py_RETURN_NONE;
}

// Return the truth table for design
PyObject* PySNLDesign_getTruthTable(PySNLDesign* self) { 
  const SNLTruthTable& truthTable =
      SNLDesignTruthTable::getTruthTable(self->object_);
  if (!truthTable.isInitialized()) {
    Py_RETURN_NONE;
  }
  PyObject* py_list = PyList_New(2); 
  PyList_SetItem(py_list, 0, PyLong_FromLong(truthTable.size()));
  for (auto mask : truthTable.bits().getChunks()) {
    PyList_SetItem(py_list, 1, PyLong_FromLong(mask));
  }
  return py_list;
}

static PyObject* PySNLDesign_dumpFullDotFile(PySNLDesign* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  std::string dotFileName(outputPath.string());
  naja::NL::SNLDesign* design = self->object_;
  naja::SnlVisualiser snl(design);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_dumpContextDotFile(PySNLDesign* self, PyObject* args) {
  char* path = NULL; 
  if (not PyArg_ParseTuple(args, "s:SELF_TYPE.METHOD", &path)) {
    setError("dumpDotFile expact a string as argument");
    return nullptr;
  }
  std::filesystem::path outputPath;
  if (path) {
    outputPath = std::filesystem::path(path);
  }
  std::string dotFileName(outputPath.string());
  naja::NL::SNLDesign* design = self->object_;
  naja::SnlVisualiser snl(design, false);
  snl.process();
  snl.getNetlistGraph().dumpDotFile(dotFileName.c_str());
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_getInstanceByIDList(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDesign.getInsatnceByIDList", &arg0)) {
    setError("malformed SNLDesign.getInsatnceByIDList method");
    return nullptr;
  }
  if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.getInsatnceByIDList method");
    return nullptr;
  }
  naja::NL::SNLDesign* design = self->object_;
  naja::NL::SNLInstance* instance = nullptr;
  for (int i=0; i<PyList_Size(arg0); ++i) {
    instance = design->getInstance(PyLong_AsLong(PyList_GetItem(arg0, i)));
    design = instance->getModel();
  }
  return PySNLInstance_Link(instance);
}

// Return list for NLID of the design
// Function to be called from Python
PyObject* PySNLDesign_getNLID(PySNLDesign* self) { 
  PyObject* py_list = PyList_New(6); 
  naja::NL::NLID id = self->object_->getNLID();
  PyList_SetItem(py_list, 0, PyLong_FromLong(id.dbID_));
  PyList_SetItem(py_list, 1, PyLong_FromLong(id.libraryID_));
  PyList_SetItem(py_list, 2, PyLong_FromLong(id.designID_));
  PyList_SetItem(py_list, 3, PyLong_FromLong(id.designObjectID_));
  PyList_SetItem(py_list, 4, PyLong_FromLong(id.instanceID_));
  PyList_SetItem(py_list, 5, PyLong_FromLong(id.bit_));
  return py_list;
}

static PyObject* PySNLDesign_getCombinatorialInputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getCombinatorialInputs, SNLDesign)
}

static PyObject* PySNLDesign_getCombinatorialOutputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getCombinatorialOutputs, SNLDesign)
}

static PyObject* PySNLDesign_getClockRelatedInputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getClockRelatedInputs, SNLDesign)
}

static PyObject* PySNLDesign_getClockRelatedOutputs(PySNLDesign*, PyObject* object) {
  GetDesignModelingRelatedObjects(SNLBitTerm, getClockRelatedOutputs, SNLDesign)
}

GetObjectMethod(SNLDesign, NLDB, getDB)
GetObjectMethod(SNLDesign, NLLibrary, getLibrary)
GetObjectByName(SNLDesign, SNLInstance, getInstance)
GetObjectByName(SNLDesign, SNLTerm, getTerm)
GetObjectByName(SNLDesign, SNLScalarTerm, getScalarTerm)
GetObjectByName(SNLDesign, SNLBusTerm, getBusTerm)
GetObjectByName(SNLDesign, SNLNet, getNet)
GetObjectByName(SNLDesign, SNLScalarNet, getScalarNet)
GetObjectByName(SNLDesign, SNLBusNet, getBusNet)
GetObjectByName(SNLDesign, SNLParameter, getParameter)
GetObjectByIndex(SNLDesign, SNLInstance, InstanceByID)
GetObjectByIndex(SNLDesign, SNLTerm, TermByID)
GetNameMethod(SNLDesign)
SetNameMethod(SNLDesign)
DirectGetIntMethod(PySNLDesign_getID, getID, PySNLDesign, SNLDesign)
DirectGetIntMethod(PySNLDesign_getRevisionCount, getRevisionCount, PySNLDesign, SNLDesign)
GetBoolAttribute(SNLDesign, isUnnamed)
GetBoolAttribute(SNLDesign, isBlackBox)
GetBoolAttribute(SNLDesign, isPrimitive)
GetBoolAttribute(SNLDesign, isLeaf)
GetBoolAttribute(SNLDesign, isAssign)
GetBoolAttribute(SNLDesign, isTopDesign)
GetBoolAttributeWithFunction(SNLDesign, isConst0, SNLDesignTruthTable::isConst0)
GetBoolAttributeWithFunction(SNLDesign, isConst1, SNLDesignTruthTable::isConst1)
GetBoolAttributeWithFunction(SNLDesign, isConst, SNLDesignTruthTable::isConst)
GetBoolAttributeWithFunction(SNLDesign, isBuf, SNLDesignTruthTable::isBuf)
GetBoolAttributeWithFunction(SNLDesign, isInv, SNLDesignTruthTable::isInv)
GetContainerMethod(SNLDesign, SNLTerm*, SNLTerms, Terms)
GetContainerMethod(SNLDesign, SNLBitTerm*, SNLBitTerms, BitTerms)
GetContainerMethod(SNLDesign, SNLScalarTerm*, SNLScalarTerms, ScalarTerms)
GetContainerMethod(SNLDesign, SNLBusTerm*, SNLBusTerms, BusTerms)
GetContainerMethod(SNLDesign, SNLNet*, SNLNets, Nets)
GetContainerMethod(SNLDesign, SNLScalarNet*, SNLScalarNets, ScalarNets)
GetContainerMethod(SNLDesign, SNLBusNet*, SNLBusNets, BusNets)
GetContainerMethod(SNLDesign, SNLBitNet*, SNLBitNets, BitNets)
GetContainerMethod(SNLDesign, SNLInstance*, SNLInstances, Instances)
GetContainerMethod(SNLDesign, SNLParameter*, SNLParameters, Parameters)

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)

PyMethodDef PySNLDesign_Methods[] = {
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  { "getID", (PyCFunction)PySNLDesign_getID, METH_NOARGS,
    "get the ID."},
  { "getRevisionCount", (PyCFunction)PySNLDesign_getRevisionCount, METH_NOARGS,
    "get revision count."},
  { "createPrimitive", (PyCFunction)PySNLDesign_createPrimitive, METH_VARARGS|METH_STATIC,
    "SNLDesign Primitive creator"},
  { "addCombinatorialArcs", (PyCFunction)PySNLDesign_addCombinatorialArcs, METH_VARARGS|METH_STATIC,
    "add combinatorial arcs"},
  { "addInputsToClockArcs", (PyCFunction)PySNLDesign_addInputsToClockArcs, METH_VARARGS|METH_STATIC,
    "add inputs to clock arcs"}, 
  { "addClockToOutputsArcs", (PyCFunction)PySNLDesign_addClockToOutputsArcs, METH_VARARGS|METH_STATIC,
    "add inputs to clock arcs"}, 
  { "addCombinatorialArcs", (PyCFunction)PySNLDesign_addCombinatorialArcs, METH_VARARGS|METH_STATIC,
    "add combinatorial arcs"},
  { "getCombinatorialInputs", (PyCFunction)PySNLDesign_getCombinatorialInputs, METH_O|METH_STATIC,
    "get combinatorial inputs of a term"},
  { "getCombinatorialOutputs", (PyCFunction)PySNLDesign_getCombinatorialOutputs, METH_O|METH_STATIC,
    "get combinatorial outputs of a term"},
  { "getClockRelatedInputs", (PyCFunction)PySNLDesign_getClockRelatedInputs, METH_O|METH_STATIC,
    "get inputs related to a clock"},
  { "getClockRelatedOutputs", (PyCFunction)PySNLDesign_getClockRelatedOutputs, METH_O|METH_STATIC,
    "get outputs related to a clock"},
  { "setTruthTable", (PyCFunction)PySNLDesign_setTruthTable, METH_VARARGS,
    "set truth table of a primitive"},
  { "getTruthTable", (PyCFunction)PySNLDesign_getTruthTable, METH_NOARGS,
    "get truth table of a primitive"},
  { "isConst0", (PyCFunction)PySNLDesign_isConst0, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant 0"},
  { "isConst1", (PyCFunction)PySNLDesign_isConst1, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant 1"},
  { "isConst", (PyCFunction)PySNLDesign_isConst, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant (1 or 0)"},
  { "isBuf", (PyCFunction)PySNLDesign_isBuf, METH_NOARGS,
    "Returns True if this design is a buffer primitive"},
  { "isInv", (PyCFunction)PySNLDesign_isInv, METH_NOARGS,
    "Returns True if this design is an inverter primitive"},  
  { "isTopDesign", (PyCFunction)PySNLDesign_isTopDesign, METH_NOARGS,
    "Returns True if this design is an inverter primitive"},  
  { "getName", (PyCFunction)PySNLDesign_getName, METH_NOARGS,
    "get SNLDesign name"},
  { "isUnnamed", (PyCFunction)PySNLDesign_isUnnamed, METH_NOARGS,
    "Returns True if the SNLDesign is unnamed"},
  { "setName", (PyCFunction)PySNLDesign_setName, METH_O,
    "Set the NLName of this SNLDesign."},
  { "isBlackBox", (PyCFunction)PySNLDesign_isBlackBox, METH_NOARGS,
    "Returns True if the SNLDesign is a Blackbox"},
  { "isPrimitive", (PyCFunction)PySNLDesign_isPrimitive, METH_NOARGS,
    "Returns True if the SNLDesign is a Primitive"},
  { "isLeaf", (PyCFunction)PySNLDesign_isLeaf, METH_NOARGS,
    "Returns True if the SNLDesign is a Leaf"},
  { "isAssign", (PyCFunction)PySNLDesign_isAssign, METH_NOARGS,
    "Returns True if the SNLDesign is an Assign"},
  { "getDB", (PyCFunction)PySNLDesign_getDB, METH_NOARGS,
    "Returns the SNLDesign owner SNLDB."},
  { "getLibrary", (PyCFunction)PySNLDesign_getLibrary, METH_NOARGS,
    "Returns the SNLDesign owner NLLibrary."},
  { "getTerm", (PyCFunction)PySNLDesign_getTerm, METH_VARARGS,
    "retrieve a SNLTerm."},
  { "getTermByID", (PyCFunction)PySNLDesign_getTermByID, METH_VARARGS,
    "retrieve a SNLTerm."},
  { "getScalarTerm", (PyCFunction)PySNLDesign_getScalarTerm, METH_VARARGS,
    "retrieve a SNLScalarTerm."},
  { "getBusTerm", (PyCFunction)PySNLDesign_getBusTerm, METH_VARARGS,
    "retrieve a SNLBusTerm."},
  { "getNet", (PyCFunction)PySNLDesign_getNet, METH_VARARGS,
    "retrieve a SNLNet."},
  { "getScalarNet", (PyCFunction)PySNLDesign_getScalarNet, METH_VARARGS,
    "retrieve a SNLScalarNet."},
  { "getBusNet", (PyCFunction)PySNLDesign_getBusNet, METH_VARARGS,
    "retrieve a SNLBusNet."},
  { "getInstance", (PyCFunction)PySNLDesign_getInstance, METH_VARARGS,
    "retrieve a SNLInstance."},
  { "getInstanceByID", (PyCFunction)PySNLDesign_getInstanceByID, METH_VARARGS,
    "retrieve a SNLInstance."},
  { "getParameter", (PyCFunction)PySNLDesign_getParameter, METH_VARARGS,
    "retrieve a SNLParameter."},
  { "getTerms", (PyCFunction)PySNLDesign_getTerms, METH_NOARGS,
    "get a container of SNLTerms."},
  { "getBitTerms", (PyCFunction)PySNLDesign_getBitTerms, METH_NOARGS,
    "get a container of SNLBitTerms."},
  { "getScalarTerms", (PyCFunction)PySNLDesign_getScalarTerms, METH_NOARGS,
    "get a container of SNLScalarTerms."},
  { "getBusTerms", (PyCFunction)PySNLDesign_getBusTerms, METH_NOARGS,
    "get a container of SNLBusTerms."},
  { "getNets", (PyCFunction)PySNLDesign_getNets, METH_NOARGS,
    "get a container of SNLNets."},
  { "getScalarNets", (PyCFunction)PySNLDesign_getScalarNets, METH_NOARGS,
    "get a container of SNLScalarNets."},
  { "getBusNets", (PyCFunction)PySNLDesign_getBusNets, METH_NOARGS,
    "get a container of SNLBusNets."},
  { "getBitNets", (PyCFunction)PySNLDesign_getBitNets, METH_NOARGS,
    "get a container of SNLBitNets."},
  { "getInstances", (PyCFunction)PySNLDesign_getInstances, METH_NOARGS,
    "get a container of SNLInstances."},
  { "getParameters", (PyCFunction)PySNLDesign_getParameters, METH_NOARGS,
    "get a container of SNLParameters."},
  { "destroy", (PyCFunction)PySNLDesign_destroy, METH_NOARGS,
    "destroy this SNLDesign."},
  { "clone", (PyCFunction)PySNLDesign_clone, METH_VARARGS,
    "clone this SNLDesign."},
  { "dumpVerilog", (PyCFunction)PySNLDesign_dumpVerilog, METH_VARARGS,
    "dump verilog file of this SNLDesign."},
  { "dumpFullDotFile", (PyCFunction)PySNLDesign_dumpFullDotFile, METH_VARARGS,
    "dump full dot file for this SNLDesign."},
  { "dumpContextDotFile", (PyCFunction)PySNLDesign_dumpContextDotFile, METH_VARARGS,
    "dump context dot file for this SNLDesign."},
  { "getInstanceByIDList", (PyCFunction)PySNLDesign_getInstanceByIDList, METH_VARARGS,
    "get instance by ID list."},
  { "getNLID", (PyCFunction)PySNLDesign_getNLID, METH_NOARGS,
    "get NLID of the design."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeNLFinalObjectWithNLIDLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}
