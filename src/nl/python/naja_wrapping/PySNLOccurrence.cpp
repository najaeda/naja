// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLOccurrence.h"
#include "PyInterface.h"
#include "PySNLDesign.h"
#include "SNLDesignObject.h"
#include "PySNLDesignObject.h"
#include "PySNLPath.h"
#include "PySNLInstance.h"
#include "PySNLInstTerm.h"
#include "SNLDesignObject.h"
#include "SNLInstance.h"
#include "SNLPath.h"
#include "SNLOccurrence.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLOccurrence, function)

static int PySNLOccurrence_Init(PySNLOccurrence* self, PyObject* args, PyObject* kwargs) {
  SNLOccurrence* snlOccurrence = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLOccurrence has three types of constructors:
  if (not PyArg_ParseTuple(args, "|OO:SNLOccurrence", &arg0, &arg1)) {
    setError("malformed SNLOccurrence create method");
    return -1;
  }
  if (arg0 == nullptr) {
    snlOccurrence = new SNLOccurrence();
  } else if (arg1 == nullptr) {
    if (IsPySNLDesignObject(arg0)) {
      snlOccurrence = new SNLOccurrence(PYSNLDesignObject_O(arg0));
    } else {
      setError("SNLOccurrence create accepts SNLDesignObject as only argument");
      return -1;
    }
  } else if (IsPySNLPath(arg0) and IsPySNLDesignObject(arg1)) {
    snlOccurrence = new SNLOccurrence(*PYSNLPath_O(arg0), PYSNLDesignObject_O(arg1));
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
    return -1;
  }
  self->object_ = snlOccurrence;
  return 0;
}

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLOccurrence) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLOccurrence)

static Py_uhash_t hashSNLPathForOccurrence(const SNLPath& path) {
  Py_uhash_t seed = 0;
  for (auto instance: path.getInstances()) {
    PYNAJA::combinePyHash(seed, PYNAJA::hashNLID(instance->getNLID()));
  }
  return seed;
}

static Py_uhash_t hashSNLOccurrence(const SNLOccurrence& occurrence) {
  Py_uhash_t seed = 0;
  auto object = occurrence.getObject();
  if (object) {
    PYNAJA::combinePyHash(seed, PYNAJA::hashNLID(object->getNLID()));
  }
  PYNAJA::combinePyHash(seed, hashSNLPathForOccurrence(occurrence.getPath()));
  return seed;
}

static Py_hash_t PySNLOccurrence_Hash(PySNLOccurrence* self) {
  if (not self->ACCESS_OBJECT) {
    setError("Attempt to call SNLOccurrence.__hash__() on an unbound object");
    return -1;
  }
  return PYNAJA::finishPyHash(hashSNLOccurrence(*self->ACCESS_OBJECT));
}

GetObjectMethod(SNLOccurrence, SNLNetComponent, getNetComponent)
GetObjectMethod(SNLOccurrence, SNLInstTerm, getInstTerm)
GetObjectMethod(SNLOccurrence, SNLInstance, getInstance)
GetObjectMethod(SNLOccurrence, SNLDesign, getDesign) // LCOV_EXCL_LINE
GetObjectMethod(SNLOccurrence, SNLPath, getPath)
GetBoolAttribute(SNLOccurrence, isInstanceOccurrence)

PyMethodDef PySNLOccurrence_Methods[] = {
  { "getNetComponent", (PyCFunction)PySNLOccurrence_getNetComponent, METH_NOARGS,
    "get the SNLNetComponent of the SNLOccurrence."},
  { "getInstTerm", (PyCFunction)PySNLOccurrence_getInstTerm, METH_NOARGS,
    "get the SNLInstTerm of the SNLOccurrence."},
  { "getInstance", (PyCFunction)PySNLOccurrence_getInstance, METH_NOARGS,
    "get the SNLInstance of the SNLOccurrence (None if the object is not an instance)."},
  { "getDesign", (PyCFunction)PySNLOccurrence_getDesign, METH_NOARGS,
    "get the design context of the SNLOccurrence."},
  { "isInstanceOccurrence", (PyCFunction)PySNLOccurrence_isInstanceOccurrence, METH_NOARGS,
    "return whether the SNLOccurrence references an SNLInstance."},
  { "getPath", (PyCFunction)PySNLOccurrence_getPath, METH_NOARGS,
    "get the SNLPath of the SNLInstTermOccurrence."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyTypeWithHash(SNLOccurrence, PySNLOccurrence_Hash)
PyTypeObjectDefinitions(SNLOccurrence)

}  // namespace PYNAJA
