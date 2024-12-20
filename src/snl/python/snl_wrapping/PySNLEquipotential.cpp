// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLEquipotential.h"
#include "PyInterface.h"
#include "SNLNetComponentOccurrence.h"
#include "PySNLNetComponentOccurrence.h"
#include "PySNLPath.h"
#include "PySNLInstTermOccurrences.h"
#include "PySNLBitTerms.h"
#include "SNLNetComponentOccurrence.h"
#include "SNLPath.h"
#include "SNLEquipotential.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLEquipotential, function)

static int PySNLEquipotential_Init(PySNLEquipotential* self, PyObject* args, PyObject* kwargs) {
  SNLEquipotential* equipotential = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLEquipotential has three types of constructors:
  if (not PyArg_ParseTuple(args, "|O:SNLEquipotential", &arg0)) {
    setError("malformed SNLEquipotential create method");
    return -1;
  }
  if (arg0 != nullptr) {
    if (IsPySNLNetComponentOccurrence(arg0)) {
      equipotential = new SNLEquipotential(*PYSNLNetComponentOccurrence_O(arg0));
    } else {
      setError("SNLEquipotential create accepts SNLNetComponentOccurrence as only argument");
      return -1;
    }
  }  else {
    setError("invalid number of parameters for Occurrence constructor.");
    return -1;
  }
  self->object_ = equipotential;
  return 0;
}

GetContainerMethod(Equipotential, BitTerm, BitTerms, Terms)
GetContainerMethodForNonPointers(Equipotential, InstTermOccurrence, InstTermOccurrences, InstTermOccurrences)

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLEquipotential) 
//LCOV_EXCL_STOP
ManagedTypeDeallocMethod(SNLEquipotential)

PyMethodDef PySNLEquipotential_Methods[] = {
  {"getTerms", (PyCFunction)PySNLEquipotential_getTerms, METH_NOARGS,
    "Returns the equi top terms."},
  {"getInstTermOccurrences", (PyCFunction)PySNLEquipotential_getInstTermOccurrences, METH_NOARGS,
    "Returns the equi inst terms."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLEquipotential)
PyTypeObjectDefinitions(SNLEquipotential)

}  // namespace PYSNL