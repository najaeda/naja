// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLOccurrence.h"
#include "PyInterface.h"
#include "SNLDesignObject.h"
#include "PySNLDesignObject.h"
#include "PySNLPath.h"
#include "SNLDesignObject.h"
#include "SNLPath.h"
#include "SNLOccurrence.h"

namespace PYSNL {

using namespace naja::SNL;

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

PyMethodDef PySNLOccurrence_Methods[] = {
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLOccurrence)
PyTypeObjectDefinitions(SNLOccurrence)

}  // namespace PYSNL