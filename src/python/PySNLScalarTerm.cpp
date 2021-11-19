#include "PySNLScalarTerm.h"

#include "PySNLDesign.h"

#include "SNLScalarTerm.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLScalarTerm_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  const char* arg1 = nullptr;
  if (not PyArg_ParseTuple(args, "O|s:SNLDB.create", &arg0, &arg1)) {
    setError("malformed SNLScalarTerm create method");
    return nullptr;
  }
  SNLName name;
  if (arg1) {
    name = SNLName(arg1);
  }

  SNLScalarTerm* design = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    design = SNLScalarTerm::create(PYSNLDesign_O(arg0), SNLTerm::Direction::Input, name);
  } else {
    setError("SNLScalarTerm create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLScalarTerm_Link(design);
}

PyMethodDef PySNLScalarTerm_Methods[] = {
  { "create", (PyCFunction)PySNLScalarTerm_create, METH_VARARGS|METH_STATIC,
    "SNLScalarTerm creator"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLScalarTerm_destroy, PySNLScalarTerm)
DBoDeallocMethod(SNLScalarTerm)

DBoLinkCreateMethod(SNLScalarTerm)
PyTypeObjectLinkPyType(SNLScalarTerm)
PyTypeObjectDefinitions(SNLScalarTerm)

}
