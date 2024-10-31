// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLNetComponentOccurrence.h"
#include "PyInterface.h"
#include "PySNLPath.h"
#include "PySNLNetComponent.h"
#include "SNLPath.h"
#include "SNLNetComponentOccurrence.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLNetComponentOccurrence, function)

static int PySNLNetComponentOccurrence_Init(PySNLNetComponentOccurrence* self, PyObject* args, PyObject* kwargs) {
  SNLNetComponentOccurrence* snlOccurrence = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLNetComponentOccurrence has three types of constructors:
  if (not PyArg_ParseTuple(args, "|OO:SNLNetComponentOccurrence", &arg0, &arg1)) {
    setError("malformed SNLNetComponentOccurrence create method");
    return -1;
  }
  if (arg0 == nullptr) {
    snlOccurrence = new SNLNetComponentOccurrence();
  } else if (arg1 == nullptr) {
    if (IsPySNLNetComponent(arg0)) {
      snlOccurrence = new SNLNetComponentOccurrence(PYSNLNetComponent_O(arg0));
    } else {
      setError("SNLNetComponentOccurrence create accepts SNLNetComponent as only argument");
      return -1;
    }
  } else if (IsPySNLPath(arg0) and IsPySNLNetComponent(arg1)) {
    snlOccurrence = new SNLNetComponentOccurrence(*PYSNLPath_O(arg0), PYSNLNetComponent_O(arg1));
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
    return -1;
  }
  self->object_ = snlOccurrence;
  return 0;
}

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLNetComponentOccurrence) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLNetComponentOccurrence)

PyMethodDef PySNLNetComponentOccurrence_Methods[] = {
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLNetComponentOccurrence)
PyTypeObjectDefinitions(SNLNetComponentOccurrence)

}  // namespace PYSNL