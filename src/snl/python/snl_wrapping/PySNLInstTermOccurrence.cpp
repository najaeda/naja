// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLInstTermOccurrence.h"
#include "PyInterface.h"
#include "SNLInstTerm.h"
#include "PySNLInstTerm.h"
#include "PySNLPath.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "SNLInstTermOccurrence.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLInstTermOccurrence, function)

static int PySNLInstTermOccurrence_Init(PySNLInstTermOccurrence* self, PyObject* args, PyObject* kwargs) {
  SNLInstTermOccurrence* snlOccurrence = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLInstTermOccurrence has three types of constructors:
  if (not PyArg_ParseTuple(args, "|OO:SNLInstTermOccurrence", &arg0, &arg1)) {
    setError("malformed SNLInstTermOccurrence create method");
    return -1;
  }
  if (arg0 == nullptr) {
    snlOccurrence = new SNLInstTermOccurrence();
  } else if (arg1 == nullptr) {
    if (IsPySNLInstTerm(arg0)) {
      snlOccurrence = new SNLInstTermOccurrence(PYSNLInstTerm_O(arg0));
    } else {
      setError("SNLInstTermOccurrence create accepts SNLInstTerm as only argument");
      return -1;
    }
  } else if (IsPySNLPath(arg0) and IsPySNLInstTerm(arg1)) {
    snlOccurrence = new SNLInstTermOccurrence(*PYSNLPath_O(arg0), PYSNLInstTerm_O(arg1));
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
    return -1;
  }
  self->object_ = snlOccurrence;
  return 0;
}

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLInstTermOccurrence) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLInstTermOccurrence)

PyMethodDef PySNLInstTermOccurrence_Methods[] = {
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLInstTermOccurrence)
PyTypeObjectDefinitions(SNLInstTermOccurrence)

}  // namespace PYSNL