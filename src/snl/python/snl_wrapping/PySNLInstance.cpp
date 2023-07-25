// Copyright 2022 The Naja Authors.
// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstance.h"

#include "PyInterface.h"
#include "PySNLDesign.h"
#include "PySNLInstTerm.h"
#include "PySNLBitTerm.h"

#include "SNLInstance.h"

namespace PYSNL {

using namespace naja::SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstance, function)

static PyObject* PySNLInstance_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OO|s:SNLDB.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstance create method");
    return nullptr;
  }
  SNLName name;
  if (arg2) {
    name = SNLName(arg2);
  }

  SNLInstance* instance = nullptr;
  SNLTRY
  if (not IsPySNLDesign(arg0)) {
    setError("SNLInstance create needs SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPySNLDesign(arg1)) {
    setError("SNLInstance create needs SNLDesign as second argument");
    return nullptr;
  }
  instance = SNLInstance::create(PYSNLDesign_O(arg0), PYSNLDesign_O(arg1), name);
  SNLCATCH
  return PySNLInstance_Link(instance);
}

static PyObject* PySNLInstance_getModel(PySNLInstance* self) {
  METHOD_HEAD("SNLInstance.getModel()")
  return PySNLDesign_Link(selfObject->getModel());
}

GetNameMethod(SNLInstance)

DBoDeallocMethod(SNLInstance)

DBoLinkCreateMethod(SNLInstance)
PyTypeInheritedObjectDefinitions(SNLInstance, SNLDesignObject)

static PyObject* PySNLInstance_getInstTerm(PySNLInstance* self, PyObject* args) {
  SNLInstTerm* obj = nullptr;
  METHOD_HEAD("SNLInstance.getInstTerm()")
  PySNLBitTerm* pyBitTerm = nullptr;
  if (PyArg_ParseTuple(args, "O!:SNLInstance.getInstTerm", &PyTypeSNLBitTerm, &pyBitTerm)) {
    SNLTRY
    auto bitTerm = PYSNLBitTerm_O(pyBitTerm);
    if (bitTerm) {
      obj = selfObject->getInstTerm(bitTerm);
    }
    SNLCATCH
  } else {
    setError("invalid number of parameters for getInstTerm.");
    return nullptr;
  }
  return PySNLInstTerm_Link(obj);
}

PyMethodDef PySNLInstance_Methods[] = {
  { "create", (PyCFunction)PySNLInstance_create, METH_VARARGS|METH_STATIC,
    "SNLInstance creator"},
  { "getName", (PyCFunction)PySNLInstance_getName, METH_NOARGS,
    "get SNLInstance name"},
  {"getModel", (PyCFunction)PySNLInstance_getModel, METH_NOARGS,
    "Returns the SNLInstance model SNLDesign."},
  {"getInstTerm", (PyCFunction)PySNLInstance_getInstTerm, METH_VARARGS,
    "Returns the SNLInstTerm corresponding to a model's SNLBitTerm."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLInstance)

}
