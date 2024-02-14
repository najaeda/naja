// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstParameter.h"

#include "PyInterface.h"
#include "PySNLInstance.h"
#include "SNLInstParameter.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstParameter, function)

GetNameMethod(SNLInstParameter)
GetObjectMethod(InstParameter, Instance, getInstance)

static PyObject* PySNLInstParameter_setValue(PySNLInstParameter* self, PyObject* arg) {
  METHOD_HEAD("PySNLInstParameter.setValue()")
  if (not PyUnicode_Check(arg)) {
    setError("SNLInstParameter.setValue() expects a string as argument");
    return nullptr;
  }
  selfObject->setValue(PyUnicode_AsUTF8(arg));
  Py_RETURN_NONE;
}

DBoDestroyAttribute(PySNLInstParameter_destroy, PySNLInstParameter)

PyMethodDef PySNLInstParameter_Methods[] = {
  { "getName", (PyCFunction)PySNLInstParameter_getName, METH_NOARGS,
    "get SNLInstParameter name"},
  { "getInstance", (PyCFunction)PySNLInstParameter_getInstance, METH_NOARGS,
    "get SNLInstParameter owner instance"},
  {"setValue", (PyCFunction)PySNLInstParameter_setValue, METH_O,
    "Set the value of this SNLInstParameter."},
  {"destroy", (PyCFunction)PySNLInstParameter_destroy, METH_NOARGS,
    "destroy this SNLInstParameter."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLInstParameter)

DBoLinkCreateMethod(SNLInstParameter)
PyTypeSNLObjectWithoutSNLIDLinkPyType(SNLInstParameter)
PyTypeObjectDefinitions(SNLInstParameter)

}
