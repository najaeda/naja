// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLDesignObject.h"

#include "SNLDesignObject.h"
#include "SNLRTLInfos.h"
#include "PySNLSourceReference.h"

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
DirectGetNLIDMethod(PySNLDesignObject_getNLID, PySNLDesignObject, SNLDesignObject)

GetContainerMethod(SNLDesignObject, SNLAttribute, SNLAttributes, Attributes)

static PyObject* PySNLDesignObject_getSourceDeclaration(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getSourceDeclaration()")
  return PYNAJA::getSourceDeclaration(selfObject->getRTLInfos());
}

static PyObject* PySNLDesignObject_getSourceOrigins(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getSourceOrigins()")
  return PYNAJA::getSourceOrigins(selfObject->getRTLInfos());
}

static PyObject* PySNLDesignObject_setSourceDeclaration(PySNLDesignObject* self, PyObject* value) {
  METHOD_HEAD("SNLDesignObject.setSourceDeclaration()")
  return PYNAJA::setSourceDeclaration(selfObject, value);
}

static PyObject* PySNLDesignObject_addSourceOrigin(PySNLDesignObject* self, PyObject* value) {
  METHOD_HEAD("SNLDesignObject.addSourceOrigin()")
  return PYNAJA::addSourceOrigin(selfObject, value);
}

static PyObject* PySNLDesignObject_clearSourceOrigins(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.clearSourceOrigins()")
  if (auto* infos = selfObject->getRTLInfos()) infos->clearSourceOrigins();
  Py_RETURN_NONE;
}

static PyObject* PySNLDesignObject_hasSourceLoc(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.hasSourceLoc()")
  auto* rtlInfos = selfObject->getRTLInfos();
  if (rtlInfos and rtlInfos->hasSourceLoc()) Py_RETURN_TRUE;
  Py_RETURN_FALSE;
}

static PyObject* PySNLDesignObject_getSourceLoc(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getSourceLoc()")
  auto* rtlInfos = selfObject->getRTLInfos();
  if (not rtlInfos or not rtlInfos->hasSourceLoc()) {
    Py_RETURN_NONE;
  }
  const auto& loc = *rtlInfos->getSourceLoc();
  return Py_BuildValue(
    "(sIIII)",
    loc.file.getString().c_str(),
    static_cast<unsigned int>(loc.line),
    static_cast<unsigned int>(loc.column),
    static_cast<unsigned int>(loc.endLine),
    static_cast<unsigned int>(loc.endColumn));
}

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
  {"getSourceDeclaration", (PyCFunction)PySNLDesignObject_getSourceDeclaration, METH_NOARGS,
    "getSourceDeclaration: language-neutral source metadata; independent of legacy source location."},
  {"getSourceOrigins", (PyCFunction)PySNLDesignObject_getSourceOrigins, METH_NOARGS,
    "getSourceOrigins: language-neutral source metadata; independent of legacy source location."},
  {"setSourceDeclaration", (PyCFunction)PySNLDesignObject_setSourceDeclaration, METH_O,
    "setSourceDeclaration: language-neutral source metadata; independent of legacy source location."},
  {"addSourceOrigin", (PyCFunction)PySNLDesignObject_addSourceOrigin, METH_O,
    "addSourceOrigin: language-neutral source metadata; independent of legacy source location."},
  {"clearSourceOrigins", (PyCFunction)PySNLDesignObject_clearSourceOrigins, METH_NOARGS,
    "clearSourceOrigins: language-neutral source metadata; independent of legacy source location."},
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {"getNLID", (PyCFunction)PySNLDesignObject_getNLID, METH_NOARGS,
    "get the NLID."},
  {"addAttribute", (PyCFunction)PySNLDesignObject_addAttribute, METH_VARARGS,
    "add an attribute to this design object."},
  {"getAttributes", (PyCFunction)PySNLDesignObject_getAttributes, METH_NOARGS,
    "get a container of SNLAttributes."},
  {"hasSourceLoc", (PyCFunction)PySNLDesignObject_hasSourceLoc, METH_NOARGS,
    "Returns whether the SNLDesignObject has source location information."},
  {"getSourceLoc", (PyCFunction)PySNLDesignObject_getSourceLoc, METH_NOARGS,
    "Returns source location as (file, line, column, end_line, end_column), or None."},
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
