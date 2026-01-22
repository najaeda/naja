// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLScalarTerm.h"

#include "PyInterface.h"
#include "PySNLDesign.h"

#include "SNLScalarTerm.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(Instance, instance, function)

static PyObject* PySNLScalarTerm_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  int intDir = 0;
  if (not PyArg_ParseTuple(args, "Oi|s:SNLDB.create", &arg0, &intDir, &arg1)) {
    setError("malformed SNLScalarTerm create method");
    return nullptr;
  }
  NLName name;
  if (arg1) {
    name = NLName(arg1);
  }

  SNLTerm::Direction direction = SNLNetComponent::Direction::DirectionEnum(intDir);

  SNLScalarTerm* term = nullptr;
  TRY
  if (IsPySNLDesign(arg0)) {
    term = SNLScalarTerm::create(PYSNLDesign_O(arg0), direction, name);
  } else {
    setError("SNLScalarTerm create accepts SNLDesign as first argument");
    return nullptr;
  }
  NLCATCH
  return PySNLScalarTerm_Link(term);
}

DirectGetIntMethod(PySNLScalarTerm_getID, getID, PySNLScalarTerm, SNLScalarTerm)
DirectGetIntMethod(PySNLScalarTerm_getFlatID, getFlatID, PySNLScalarTerm, SNLScalarTerm)

PyMethodDef PySNLScalarTerm_Methods[] = {
  { "create", (PyCFunction)PySNLScalarTerm_create, METH_VARARGS|METH_STATIC,
    "SNLScalarTerm creator"},
  { "getID", (PyCFunction)PySNLScalarTerm_getID, METH_NOARGS,
    "Get the NLID::DesignObjectID of the term."},
  { "getFlatID", (PyCFunction)PySNLScalarTerm_getFlatID, METH_NOARGS,
    "Get the flat ID of the term."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDeallocMethod(SNLScalarTerm)

DBoLinkCreateMethod(SNLScalarTerm)
PyTypeNLFinalObjectWithNLIDLinkPyType(SNLScalarTerm)
PyTypeObjectDefinitions(SNLScalarTerm)

}