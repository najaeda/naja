#include "PySNLDesign.h"

#include "PySNLLibrary.h"

#include "SNLDesign.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLDesign_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    //PyErr_SetString("");
    return nullptr;
  }
  SNLDesign* design = nullptr;
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }
  if (IsPySNLLibrary(arg0)) {
    design = SNLDesign::create(PYSNLLibrary_O(arg0), name);
  } else {
    //PyErr_SetString("");
    return nullptr;
  }
  return PySNLDesign_Link(design);
}

PyMethodDef PySNLDesign_Methods[] = {
  { "create", (PyCFunction)PySNLDesign_create, METH_VARARGS|METH_STATIC,
    "SNLDesign creator"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLDesign_destroy, PySNLDesign)
DBoDeallocMethod(SNLDesign)

DBoLinkCreateMethod(SNLDesign)
PyTypeObjectLinkPyType(SNLDesign)
PyTypeObjectDefinitions(SNLDesign)

}
