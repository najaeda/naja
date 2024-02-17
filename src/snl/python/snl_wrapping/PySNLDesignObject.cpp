// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesignObject.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, function)

static PyObject* PySNLDesignObject_setName(PySNLDesignObject* self, PyObject* arg) {
  METHOD_HEAD("PySNLDesignObject.setName()")
  if (not PyUnicode_Check(arg)) {
    setError("SNLDesignObject.setName() expects a string as argument");
    return nullptr;
  }
  selfObject->setName(SNLName(PyUnicode_AsUTF8(arg)));
  Py_RETURN_NONE;
}

GetObjectMethod(DesignObject, Design, getDesign)

DBoDeallocMethod(SNLDesignObject)

DBoLinkCreateMethod(SNLDesignObject)
DBoDestroyAttribute(PySNLDesignObject_destroy, PySNLDesignObject)

PyTypeObjectDefinitions(SNLDesignObject)

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {"setName", (PyCFunction)PySNLDesignObject_setName, METH_O,
    "Set the SNLName of this SNLDesignObject."},
  {"destroy", (PyCFunction)PySNLDesignObject_destroy, METH_NOARGS,
    "Destroy the associated SNLDesignObject."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLDesignObject)

}
