// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesign.h"

#include "PyInterface.h"
#include "PySNLDB.h"
#include "PySNLLibrary.h"
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
#include "SNLVRLDumper.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesign, function)

static PyObject* PySNLDesign_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDesign.create", &arg0, &arg1)) {
    setError("malformed SNLDesign create method");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLDesign* design = nullptr;
  SNLTRY
  if (IsPySNLLibrary(arg0)) {
    design = SNLDesign::create(PYSNLLibrary_O(arg0), name);
  } else {
    setError("SNLDesign create accepts SNLLibrary as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_createPrimitive(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    setError("malformed SNLDesign createPrimitive method");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLDesign* design = nullptr;
  SNLTRY
  if (IsPySNLLibrary(arg0)) {
    design = SNLDesign::create(PYSNLLibrary_O(arg0), SNLDesign::Type::Primitive, name);
  } else {
    setError("SNLDesign createPrimitive accepts SNLLibrary as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_clone(PySNLDesign* self, PyObject* args) {
  const char* arg0 = nullptr;
  if (not PyArg_ParseTuple(args, "|s:SNLDesign.clone", &arg0)) {
    setError("malformed SNLDesign.clone method");
    return nullptr;
  }
  SNLName name;
  if (arg0) {
    name = SNLName(arg0);
  }

  SNLDesign* newDesign = nullptr;
  METHOD_HEAD("SNLDesign.clone()")
  SNLTRY
  newDesign = selfObject->clone(name);
  SNLCATCH
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
  SNLTRY
  SNLVRLDumper dumper;
  dumper.setTopFileName(arg1);
  dumper.dumpDesign(selfObject, std::filesystem::path(arg0));
  SNLCATCH
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
  SNLTRY
  SNLDesignModeling::addCombinatorialArcs(terms0, terms1);
  SNLCATCH
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

GetObjectMethod(Design, DB, getDB)
GetObjectMethod(Design, Library, getLibrary)
GetObjectByName(Design, Instance)
GetObjectByName(Design, Term)
GetObjectByName(Design, ScalarTerm)
GetObjectByName(Design, BusTerm)
GetObjectByName(Design, Net)
GetObjectByName(Design, ScalarNet)
GetObjectByName(Design, BusNet)
GetObjectByName(Design, Parameter)
GetNameMethod(SNLDesign)
GetBoolAttribute(Design, isAnonymous)
GetBoolAttribute(Design, isBlackBox)
GetBoolAttribute(Design, isPrimitive)
GetBoolAttribute(Design, isAssign)
GetBoolAttributeWithFunction(Design, isConst0, SNLDesignTruthTable::isConst0)
GetBoolAttributeWithFunction(Design, isConst1, SNLDesignTruthTable::isConst1)
GetBoolAttributeWithFunction(Design, isBuf, SNLDesignTruthTable::isBuf)
GetBoolAttributeWithFunction(Design, isInv, SNLDesignTruthTable::isInv)
GetContainerMethod(Design, Term, Terms, Terms)
GetContainerMethod(Design, BitTerm, BitTerms, BitTerms)
GetContainerMethod(Design, ScalarTerm, ScalarTerms, ScalarTerms)
GetContainerMethod(Design, BusTerm, BusTerms, BusTerms)
GetContainerMethod(Design, Net, Nets, Nets)
GetContainerMethod(Design, ScalarNet, ScalarNets, ScalarNets)
GetContainerMethod(Design, BusNet, BusNets, BusNets)
GetContainerMethod(Design, BitNet, BitNets, BitNets)
GetContainerMethod(Design, Instance, Instances, Instances)
GetContainerMethod(Design, Parameter, Parameters, Parameters)

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)

PyMethodDef PySNLDesign_Methods[] = {
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
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
  { "isConst0", (PyCFunction)PySNLDesign_isConst0, METH_NOARGS,
    "Returns True if this desgin is a primitive driving a constant 0"},
  { "isConst1", (PyCFunction)PySNLDesign_isConst1, METH_NOARGS,
    "Returns True if this design is a primitive driving a constant 1"},
  { "isBuf", (PyCFunction)PySNLDesign_isBuf, METH_NOARGS,
    "Returns True if this design is a buffer primitive"},
  { "isInv", (PyCFunction)PySNLDesign_isInv, METH_NOARGS,
    "Returns True if this design is an inverter primitive"},  
  { "getName", (PyCFunction)PySNLDesign_getName, METH_NOARGS,
    "get SNLDesign name"},
  { "isAnonymous", (PyCFunction)PySNLDesign_isAnonymous, METH_NOARGS,
    "Returns True if the SNLDesign is anonymous"},
  { "isBlackBox", (PyCFunction)PySNLDesign_isBlackBox, METH_NOARGS,
    "Returns True if the SNLDesign is a Blackbox"},
  { "isPrimitive", (PyCFunction)PySNLDesign_isPrimitive, METH_NOARGS,
    "Returns True if the SNLDesign is a Primitive"},
  { "isAssign", (PyCFunction)PySNLDesign_isAssign, METH_NOARGS,
    "Returns True if the SNLDesign is an Assign"},
  { "getDB", (PyCFunction)PySNLDesign_getDB, METH_NOARGS,
    "Returns the SNLDesign owner SNLDB."},
  { "getLibrary", (PyCFunction)PySNLDesign_getLibrary, METH_NOARGS,
    "Returns the SNLDesign owner SNLLibrary."},
  { "getTerm", (PyCFunction)PySNLDesign_getTerm, METH_VARARGS,
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
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeSNLFinalObjectWithSNLIDLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}