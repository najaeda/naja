// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "PySNLPath.h"

#include "NLID.h"

#include "SNLInstance.h"
#include "SNLPath.h"

#include "PyInterface.h"
#include "PySNLInstance.h"

namespace PYNAJA {

using namespace naja::NL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLPath, function)

static int PySNLPath_Init(PySNLPath* self, PyObject* args, PyObject* kwargs) {
  SNLPath* snlPath = nullptr;
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;

  //SNLPath has three types of constructors:
  if (not PyArg_ParseTuple(args, "|OO:SNLPath", &arg0, &arg1)) {
    setError("malformed SNLPath create method");
    return -1;
  }
  if (arg0 == nullptr) {
    snlPath = new SNLPath();
  } else if (arg1 == nullptr) {
    if (IsPySNLInstance(arg0)) {
      snlPath = new SNLPath(PYSNLInstance_O(arg0));
    } else {
      setError("SNLPath create accepts SNLInstance as first argument");
      return -1;
    }
  } else if (IsPySNLPath(arg0) and IsPySNLInstance(arg1)) {
    snlPath = new SNLPath(*PYSNLPath_O(arg0), PYSNLInstance_O(arg1));
  } else if (IsPySNLInstance(arg0) and IsPySNLPath(arg1)) {
    snlPath = new SNLPath(PYSNLInstance_O(arg0), *PYSNLPath_O(arg1));
  } else {
    setError("invalid number of parameters for Path constructor.");
    return -1;
  }
  self->object_ = snlPath;
  return 0;
}

// Function to be called from Python
PyObject* PySNLPath_getPathIDs(PySNLPath* self, PyObject* args) { 
  std::vector<naja::NL::NLID::DesignObjectID> vec = self->object_->getPathIDs();
  PyObject* py_list = PyList_New(vec.size()); 
  for (size_t i = 0; i < vec.size(); ++i) { 
    PyList_SetItem(py_list, i, PyLong_FromLong(vec[i])); 
  } 
  return py_list;
}

ManagedTypeLinkCreateMethod(SNLPath) 
ManagedTypeDeallocMethod(SNLPath)

GetBoolAttribute(SNLPath, empty)
GetSizetAttribute(SNLPath, size)
GetObjectMethod(SNLPath, SNLInstance, getHeadInstance)
GetObjectMethod(SNLPath, SNLInstance, getTailInstance)
GetObjectMethod(SNLPath, SNLPath, getHeadPath)
GetObjectMethod(SNLPath, SNLPath, getTailPath)

PyMethodDef PySNLPath_Methods[] = {
  { "empty", (PyCFunction)PySNLPath_empty, METH_NOARGS,
    "Returns True if this path is empty"},
  { "getHeadInstance", (PyCFunction)PySNLPath_getHeadInstance, METH_NOARGS,
    "Returns the head instance of this path"},
  { "getTailInstance", (PyCFunction)PySNLPath_getTailInstance, METH_NOARGS,
    "Returns the tail instance of this path"},
  { "getHeadPath", (PyCFunction)PySNLPath_getHeadPath, METH_NOARGS,
    "Returns the head path of this path"},
  { "getTailPath", (PyCFunction)PySNLPath_getTailPath, METH_NOARGS,
    "Returns the tail path of this path"},
  { "size", (PyCFunction)PySNLPath_size, METH_NOARGS,
    "Returns the size of this path"},
  { "getPathIDs", (PyCFunction)PySNLPath_getPathIDs, METH_NOARGS,
    "Returns the ids of the path"},
  {NULL, NULL, 0, NULL} /* sentinel */
};

PyTypeManagedNLObjectWithoutNLIDLinkPyType(SNLPath)
PyTypeObjectDefinitions(SNLPath)

}  // namespace PYNAJA
