// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesignObject.h"

#include "PyInterface.h"

#include "PySNLDesign.h"
#include "PySNLAttributes.h"
#include "PySNLAttribute.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, function)

SetNameMethod(SNLDesignObject)

GetObjectMethod(SNLDesignObject, SNLDesign, getDesign)

DBoDestroyAttribute(PySNLDesignObject_destroy, PySNLDesignObject)

PyTypeObjectDefinitions(SNLDesignObject)

GetBoolAttribute(SNLDesignObject, isUnnamed)

GetContainerMethod(SNLDesignObject, SNLAttribute, SNLAttributes, Attributes)

static PyObject* PySNLDesignObject_addAttribute(PySNLDesignObject* self, PyObject* args) {
  METHOD_HEAD("SNLDesignObject.addAttribute()")
  PySNLAttribute* pyAttribute = nullptr;
  if (PyArg_ParseTuple(args, "O!", &PyTypeSNLAttribute, &pyAttribute)) {
    auto attribute = PYSNLAttribute_O(pyAttribute);
    SNLAttributes::addAttribute(selfObject, *attribute);
  } else {
    setError("invalid number of parameters for addAttribute.");
    return nullptr;
  }
  Py_RETURN_NONE;
}

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {"addAttribute", (PyCFunction)PySNLDesignObject_addAttribute, METH_VARARGS,
    "add an attribute to this design object."},
  {"getAttributes", (PyCFunction)PySNLDesignObject_getAttributes, METH_NOARGS,
    "get a container of SNLAttributes."},
  {"isUnnamed", (PyCFunction)PySNLDesignObject_isUnnamed, METH_NOARGS,
    "Returns whether the SNLDesignObject is unnamed."},
  {"setName", (PyCFunction)PySNLDesignObject_setName, METH_O,
    "Set the NLName of this SNLDesignObject."},
  {"destroy", (PyCFunction)PySNLDesignObject_destroy, METH_NOARGS,
    "Destroy the associated SNLDesignObject."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

PyTypeNLAbstractObjectWithNLIDLinkPyType(SNLDesignObject)

}