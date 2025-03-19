// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstParameter.h"

#include "PyInterface.h"
#include "PySNLInstance.h"
#include "PySNLParameter.h"
#include "SNLInstParameter.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstParameter, function)

static PyObject* PySNLInstParameter_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OOs:SNLInstParameter.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstParameter create method");
    return nullptr;
  }
  SNLInstParameter* instParameter = nullptr;
  TRY
  if (not IsPySNLInstance(arg0)) {
    setError("SNLInstParameter create needs SNLInstance as first argument");
    return nullptr;
  }
  if (not IsPySNLParameter(arg1)) {
    setError("SNLInstParameter create needs SNLParameter as second argument");
    return nullptr;
  }
  instParameter = SNLInstParameter::create(PYSNLInstance_O(arg0), PYSNLParameter_O(arg1), arg2);
  NLCATCH
  return PySNLInstParameter_Link(instParameter);
}

GetNameMethod(SNLInstParameter)
GetObjectMethod(SNLInstParameter, SNLInstance, getInstance)

static PyObject* PySNLInstParameter_setValue(PySNLInstParameter* self, PyObject* arg) {
  METHOD_HEAD("PySNLInstParameter.setValue()")
  if (not PyUnicode_Check(arg)) {
    setError("SNLInstParameter.setValue() expects a string as argument");
    return nullptr;
  }
  selfObject->setValue(PyUnicode_AsUTF8(arg));
  Py_RETURN_NONE;
}

GetStringAttribute(InstParameter, getValue)
DBoDestroyAttribute(PySNLInstParameter_destroy, PySNLInstParameter)

PyMethodDef PySNLInstParameter_Methods[] = {
  { "create", (PyCFunction)PySNLInstParameter_create, METH_VARARGS|METH_STATIC,
    "SNLInstParameter creator"},
  { "getName", (PyCFunction)PySNLInstParameter_getName, METH_NOARGS,
    "get SNLInstParameter name"},
  { "getInstance", (PyCFunction)PySNLInstParameter_getInstance, METH_NOARGS,
    "get SNLInstParameter owner instance"},
  { "getValue", (PyCFunction)PySNLInstParameter_getValue, METH_NOARGS,
    "get SNLInstParameter value"},
  { "setValue", (PyCFunction)PySNLInstParameter_setValue, METH_O,
    "Set the value of this SNLInstParameter."},
  { "destroy", (PyCFunction)PySNLInstParameter_destroy, METH_NOARGS,
    "destroy this SNLInstParameter."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLInstParameter)

DBoLinkCreateMethod(SNLInstParameter)
PyTypeNLObjectWithoutNLIDLinkPyType(SNLInstParameter)
PyTypeObjectDefinitions(SNLInstParameter)

}