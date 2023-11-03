// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/xtofalex/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesignObject.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, function)

static PyObject* PySNLDesignObject_getDesign(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getDesign()")
  return PySNLDesign_Link(selfObject->getDesign());
}

DBoDeallocMethod(SNLDesignObject)

DBoLinkCreateMethod(SNLDesignObject)
PyTypeObjectDefinitions(SNLDesignObject)

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeSNLObjectWithSNLIDLinkPyType(SNLDesignObject)

}
