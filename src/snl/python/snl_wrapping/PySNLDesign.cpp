// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesign.h"

#include "PyInterface.h"
#include "PySNLLibrary.h"
#include "PySNLScalarTerm.h"
#include "PySNLBusTerm.h"
#include "PySNLScalarNet.h"
#include "PySNLBusNet.h"
#include "PySNLInstance.h"
#include "PySNLInstTerm.h"
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
#include "PySNLInstTerms.h"
#include "PySNLScalarTerms.h"
#include "PySNLBusTerms.h"
#include "PySNLNets.h"
#include "PySNLBitNets.h"
#include "PySNLInstances.h"

#include "SNLDesign.h"
#include "SNLDesignModeling.h"

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
    setError("malformed Primitive SNLDesign create method");
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
    setError("SNLDesign create accepts SNLLibrary as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLDesign_Link(design);
}

static PyObject* PySNLDesign_addCombinatorialArcs(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addCombinatorialDependency", &arg0, &arg1)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
    return nullptr;
  }
  SNLDesignModeling::BitTerms terms0;
  SNLDesignModeling::BitTerms terms1;
  if (IsPySNLBitTerm(arg0)) {
    terms0.push_back(PYSNLBitTerm_O(arg0));
  } else if (not PyList_Check(arg0)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
    return nullptr;
  }
  if (IsPySNLBitTerm(arg1)) {
    terms1.push_back(PYSNLBitTerm_O(arg1));
  } else if (not PyList_Check(arg1)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
    return nullptr;
  }
  if (terms0.empty()) {
    for (int i=0; i<PyList_Size(arg0); ++i) {
      PyObject* object0 = PyList_GetItem(arg0, i);
      if (not IsPySNLBitTerm(object0)) {
        setError("malformed SNLDesign.addCombinatorialDependency method");
      }
      terms0.push_back(PYSNLBitTerm_O(object0));
    }
  }
  if (terms1.empty()) {
    for (int j=0; j<PyList_Size(arg1); ++j) {
      PyObject* object1 = PyList_GetItem(arg1, j);
      if (not IsPySNLBitTerm(object1)) {
        setError("malformed SNLDesign.addCombinatorialDependency method");
      }
      terms1.push_back(PYSNLBitTerm_O(object1));
    }
  }
  SNLTRY
  SNLDesignModeling::addCombinatorialArcs(terms0, terms1);
  SNLCATCH
  Py_RETURN_NONE;
}

static PyObject* PySNLDesign_getCombinatorialInputs(PySNLDesign* self, PyObject* output) {
  if (not IsPySNLBitTerm(output)) {
    setError("malformed SNLDesign.getCombinatorialInputs method");
  }
  auto outputTerm = PYSNLBitTerm_O(output);
  PySNLBitTerms* pyObjects = nullptr;
  SNLTRY
  auto objects = new naja::NajaCollection<SNLBitTerm*>(SNLDesignModeling::getCombinatorialInputs(outputTerm));
  pyObjects = PyObject_NEW(PySNLBitTerms, &PyTypeSNLBitTerms);
  if (not pyObjects) return nullptr;
  pyObjects->object_ = objects;
  SNLCATCH
  return (PyObject*)pyObjects;
}

static PyObject* PySNLDesign_getCombinatorialOutputs(PySNLDesign* self, PyObject* input) {
  if (IsPySNLBitTerm(input)) {
    auto inputTerm = PYSNLBitTerm_O(input);
    SNLTRY
    auto objects = new naja::NajaCollection<SNLBitTerm*>(SNLDesignModeling::getCombinatorialOutputs(inputTerm));
    auto pyObjects = PyObject_NEW(PySNLBitTerms, &PyTypeSNLBitTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    return (PyObject*)pyObjects;
    SNLCATCH
  } else if (IsPySNLInstTerm(input)) {
    auto inputITerm = PYSNLInstTerm_O(input);
    SNLTRY
    auto objects = new naja::NajaCollection<SNLInstTerm*>(SNLDesignModeling::getCombinatorialOutputs(inputITerm));
    auto pyObjects = PyObject_NEW(PySNLInstTerms, &PyTypeSNLInstTerms);
    if (not pyObjects) return nullptr;
    pyObjects->object_ = objects;
    return (PyObject*)pyObjects;
    SNLCATCH

  }
  setError("malformed SNLDesign.getCombinatorialOutputs method");
  return nullptr;
}

GetObjectMethod(Design, Library)
GetObjectByName(Design, Instance)
GetObjectByName(Design, Term)
GetObjectByName(Design, ScalarTerm)
GetObjectByName(Design, BusTerm)
GetObjectByName(Design, Net)
GetObjectByName(Design, ScalarNet)
GetObjectByName(Design, BusNet)
GetNameMethod(SNLDesign)
GetContainerMethod(Design, Term, Terms)
GetContainerMethod(Design, BitTerm, BitTerms)
GetContainerMethod(Design, ScalarTerm, ScalarTerms)
GetContainerMethod(Design, BusTerm, BusTerms)
GetContainerMethod(Design, Net, Nets)
GetContainerMethod(Design, BitNet, BitNets)
GetContainerMethod(Design, Instance, Instances)

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)

PyMethodDef PySNLDesign_Methods[] = {
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  { "createPrimitive", (PyCFunction)PySNLDesign_createPrimitive, METH_VARARGS|METH_STATIC,
    "SNLDesign Primitive creator"},
  { "addCombinatorialArcs", (PyCFunction)PySNLDesign_addCombinatorialArcs, METH_VARARGS,
    "add combinatorial arcs"},
  { "getCombinatorialInputs", (PyCFunction)PySNLDesign_getCombinatorialInputs, METH_O,
    "get combinatorial inputs of a term"},
  { "getCombinatorialOutputs", (PyCFunction)PySNLDesign_getCombinatorialOutputs, METH_O,
    "get combinatorial outputs of a term"},
  { "getName", (PyCFunction)PySNLDesign_getName, METH_NOARGS,
    "get SNLDesign name"},
  {"getLibrary", (PyCFunction)PySNLDesign_getLibrary, METH_NOARGS,
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
  { "getBitNets", (PyCFunction)PySNLDesign_getBitNets, METH_NOARGS,
    "get a container of SNLBitNets."},
  { "getInstances", (PyCFunction)PySNLDesign_getInstances, METH_NOARGS,
    "get a container of SNLInstances."},
  {"destroy", (PyCFunction)PySNLDesign_destroy, METH_NOARGS,
    "destroy this SNLDesign."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeSNLObjectWithSNLIDLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}
