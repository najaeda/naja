#include "PySNLLibrary.h"

#include "PySNLDB.h"

#include "SNLLibrary.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLLibrary_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    //PyErr_SetString("");
    return nullptr;
  }
  SNLLibrary* lib = nullptr;
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }
  if (IsPySNLDB(arg0)) {
    lib = SNLLibrary::create(PYSNLDB_O(arg0), name);
  } else if (IsPySNLLibrary(arg0)) {
    lib = SNLLibrary::create(PYSNLLibrary_O(arg0), name);
  } else {
    return nullptr;
  }
  return PySNLLibrary_Link(lib);
}

PyMethodDef PySNLLibrary_Methods[] = {
  { "create", (PyCFunction)PySNLLibrary_create, METH_VARARGS|METH_STATIC,
    "SNLLibrary creator"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLLibrary_destroy, PySNLLibrary)
DBoDeallocMethod(SNLLibrary)

DBoLinkCreateMethod(SNLLibrary)
PyTypeObjectLinkPyType(SNLLibrary)
PyTypeObjectDefinitions(SNLLibrary)

}
