// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLUniquifier.h"
#include "PyInterface.h"
#include "PySNLInstances.h"
#include "PySNLPath.h"
#include "SNLInstTerm.h"
#include "SNLPath.h"
#include "Utils.h"

namespace PYSNL {

using namespace naja::SNL;
using namespace naja::BNE;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLUniquifier, function)

static int PySNLUniquifier_Init(PySNLUniquifier* self, PyObject* args, PyObject* kwargs) {
  SNLUniquifier* uniquifier = nullptr;
  PyObject* arg0 = nullptr;

  //SNLUniquifier has three types of constructors:
  if (not PyArg_ParseTuple(args, "|O:SNLUniquifier", &arg0)) {
    setError("malformed SNLUniquifier create method");
    return -1;
  }
  if (arg0 != nullptr) {
    if (IsPySNLPath(arg0)) {
      uniquifier = new SNLUniquifier(*PYSNLPath_O(arg0));
      uniquifier->process();
    } else {
      setError("SNLUniquifier create accepts SNLPath as only argument");
      return -1;
    }
  } else {
    setError("invalid number of parameters for Uniquifier constructor.");
    return -1;
  }
  self->object_ = uniquifier;
  return 0;
}

//LCOV_EXCL_START
ManagedTypeLinkCreateMethod(SNLUniquifier) 
//LCOV_EXCL_STOP
//DBoLinkCreateMethod(SNLUniquifier)
ManagedTypeDeallocMethod(SNLUniquifier)

GetContainerMethod(Uniquifier, Instance, Instances, PathUniqCollection)

PyMethodDef PySNLUniquifier_Methods[] = {
  { "getPathUniqCollection", (PyCFunction)PySNLUniquifier_getPathUniqCollection, METH_NOARGS,
    "get the instances that contain the uniquified path."},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLUniquifier)
PyTypeObjectDefinitions(SNLUniquifier)

}  // namespace PYSNL