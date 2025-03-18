// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesignObject.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, function)

SetNameMethod(SNLDesignObject)

GetObjectMethod(SNLDesignObject, SNLDesign, getDesign)

DBoDestroyAttribute(PySNLDesignObject_destroy, PySNLDesignObject)

PyTypeObjectDefinitions(SNLDesignObject)

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {"setName", (PyCFunction)PySNLDesignObject_setName, METH_O,
    "Set the NLName of this SNLDesignObject."},
  {"destroy", (PyCFunction)PySNLDesignObject_destroy, METH_NOARGS,
    "Destroy the associated SNLDesignObject."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLDesignObject)

}
