// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLPath.h"
#include "PyInterface.h"
#include "PySNLInstance.h"
#include "SNLInstance.h"
#include "SNLPath.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLPath, function)

static PyObject* PySNLPath_createWithInstance(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLPath.createWithInstance", &arg)) {
    setError("malformed SNLPath creation with instance method");
    return nullptr;
  }
  SNLPath* snlpath = nullptr;
  SNLTRY
  if (IsPySNLInstance(arg)) {
    snlpath = new SNLPath(PYSNLInstance_O(arg));
  } else {
    setError("SNLPath create accepts SNLInstance as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLPath_Link(snlpath);
}

static PyObject* PySNLPath_createWithHeadAndInstance(PyObject*,
                                                     PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "OO:SNLPath.createWithHeadAndInstance", &arg0,
                           &arg1)) {
    setError("malformed SNLPath int value creation method");
    return nullptr;
  }
  SNLPath* path = nullptr;
  SNLInstance* inst = nullptr;
  SNLPath* snlpath = nullptr;
  SNLTRY
  if (IsPySNLPath(arg0)) {
    path = PYSNLPath_O(arg0);
  } else {
    setError("SNLPath create accepts SNLPath as first argument");
    return nullptr;
  }
  if (IsPySNLInstance(arg1)) {
    inst = PYSNLInstance_O(arg1);
  } else {
    setError("SNLPath create accepts SNLInstance as second argument");
    return nullptr;
  }
  snlpath = new SNLPath(*path, PYSNLInstance_O(arg1));
  SNLCATCH
  return PySNLPath_Link(snlpath);
}

static void PySNLPath_DeAlloc(PySNLPath* self) {
  if (self->ACCESS_OBJECT) {
    delete self->ACCESS_OBJECT;
  }
  PyObject_DEL(self);
}

PyMethodDef PySNLPath_Methods[] = {
    {"createWithInstance", (PyCFunction)PySNLPath_createWithInstance,
     METH_VARARGS | METH_STATIC, "SNLPath creator for instance"},
    {"createWithHeadAndInstance",
     (PyCFunction)PySNLPath_createWithHeadAndInstance,
     METH_VARARGS | METH_STATIC, "SNLPath creator for head path and instance"},
    {"destroy", (PyCFunction)PySNLPath_DeAlloc, METH_NOARGS,
     "Deallocate the SNLPath object"},
    {NULL, NULL, 0, NULL} /* sentinel */
};

DBoLinkCreateMethod(SNLPath) 
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLPath)
PyTypeObjectDefinitions(SNLPath)

}  // namespace PYSNL
