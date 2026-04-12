// SPDX-FileCopyrightText: 2026 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLBundleTerm.h"

#include "PyInterface.h"
#include "PySNLTerm.h"
#include "PySNLTerms.h"

#include "SNLBundleTerm.h"

namespace PYNAJA {

using namespace naja::NL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(SNLBundleTerm, function)

static PyObject* PySNLBundleTerm_getMember(PySNLBundleTerm* self, PyObject* arg) {
  METHOD_HEAD("SNLBundleTerm.getMember()")
  if (not PyLong_Check(arg)) {
    setError("SNLBundleTerm.getMember() expects an integer argument");
    return nullptr;
  }
  auto index = static_cast<size_t>(PyLong_AsSize_t(arg));
  return PySNLTerm_Link(selfObject->getMember(index));
}

DirectGetNumericMethod(PySNLBundleTerm_getID, getID, PySNLBundleTerm, SNLBundleTerm)
DirectGetNumericMethod(PySNLBundleTerm_getFlatID, getFlatID, PySNLBundleTerm, SNLBundleTerm)
DirectGetNumericMethod(PySNLBundleTerm_getNumMembers, getNumMembers, PySNLBundleTerm, SNLBundleTerm)
GetContainerMethodWithMethodName(BundleTerm, Term, getMembers)

PyMethodDef PySNLBundleTerm_Methods[] = {
  { "getID", (PyCFunction)PySNLBundleTerm_getID, METH_NOARGS,
    "Get the NLID::DesignObjectID of the term."},
  { "getFlatID", (PyCFunction)PySNLBundleTerm_getFlatID, METH_NOARGS,
    "Get the flat ID of the term."},
  { "getNumMembers", (PyCFunction)PySNLBundleTerm_getNumMembers, METH_NOARGS,
    "Get the number of bundle members."},
  { "getMember", (PyCFunction)PySNLBundleTerm_getMember, METH_O,
    "Get a bundle member by index."},
  { "getMembers", (PyCFunction)PySNLBundleTerm_getMembers, METH_NOARGS,
    "Get bundle members."},
  {NULL, NULL, 0, NULL}
};

DBoDeallocMethod(SNLBundleTerm)
DBoLinkCreateMethod(SNLBundleTerm)
PyTypeNLFinalObjectWithNLIDLinkPyType(SNLBundleTerm)
PyTypeObjectDefinitions(SNLBundleTerm)

}
