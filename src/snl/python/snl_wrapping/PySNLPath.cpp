// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLPath.h"
#include "PyInterface.h"
#include "PySNLInstance.h"
#include "SNLInstance.h"
#include "SNLPath.h"

namespace PYSNL {

using namespace naja::SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLPath, function)

static int PySNLPath_Init(PySNLPath* self, PyObject* args, PyObject* kwargs) {
  SNLPath* snlPath = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLPath has three types of constructors:
  if (PyArg_ParseTuple(args, ":SNLPath")) {
    snlPath = new SNLPath();
  } else if (PyArg_ParseTuple(args, "O:SNLPath", &arg0)) {
    if (IsPySNLInstance(arg0)) {
      snlPath = new SNLPath(PYSNLInstance_O(arg0));
    } else {
      setError("SNLPath create accepts SNLInstance as first argument");
      return -1;
    }
  } else if (PyArg_ParseTuple(args, "OO:SNLPath", &arg0, &arg1)) {
    if (IsPySNLPath(arg0) and IsPySNLInstance(arg1)) {
      snlPath = new SNLPath(*PYSNLPath_O(arg0), PYSNLInstance_O(arg1));
    } else if (IsPySNLInstance(arg0) and IsPySNLPath(arg1)) {
      snlPath = new SNLPath(PYSNLInstance_O(arg0), *PYSNLPath_O(arg1));
    } else {
      setError("invalid number of parameters for Path constructor.");
      return -1;
    }
  } else {
    setError("malformed SNLPath create method");
    return -1;
  }
  std::cerr << "PySNLPath_Init" << snlPath->getString() << std::endl;
  self->object_ = snlPath;
  return 0;
}

ManagedTypeDeallocMethod(SNLPath)

GetBoolAttribute(Path, empty)

PyMethodDef PySNLPath_Methods[] = {
  { "empty", (PyCFunction)PySNLPath_empty, METH_NOARGS,
    "Returns True if this path is empty"},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedSNLObjectWithoutSNLIDLinkPyType(SNLPath)
PyTypeObjectDefinitions(SNLPath)

}  // namespace PYSNL