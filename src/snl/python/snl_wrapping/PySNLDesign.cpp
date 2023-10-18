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
#include "PySNLTerms.h"
#include "PySNLBitTerms.h"
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

static PyObject* PySNLDesign_addCombinatorialDependency(PySNLDesign* self, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLDesign.addCombinatorialDependency", &arg0, &arg1)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
    return nullptr;
  }
  if (not IsPySNLBitTerm(arg0)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
  }
  if (not IsPySNLBitTerm(arg1)) {
    setError("malformed SNLDesign.addCombinatorialDependency method");
  }
  auto term0 = PYSNLBitTerm_O(arg0);
  auto term1 = PYSNLBitTerm_O(arg1);
  SNLDesignModeling::addCombinatorialDependency(term0, term1);
  Py_RETURN_NONE;
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
  { "addCombinatorialDependency", (PyCFunction)PySNLDesign_addCombinatorialDependency, METH_VARARGS,
    "add combinatorial dependency"},
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
