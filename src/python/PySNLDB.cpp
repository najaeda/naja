#include "PySNLDB.h"

#include "PySNLUniverse.h"

#include "SNLDB.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.create", &arg)) {
    //PyErr_SetString("");
    return nullptr;
  }
  auto universe = PYSNLUNIVERSE_O(arg);
  auto db = SNL::SNLDB::create(universe);
  return PySNLDB_Link(db);
}

PyMethodDef PySNLDB_Methods[] = {
  { "create", (PyCFunction)PySNLDB_create, METH_VARARGS|METH_STATIC,
    "create a SNL DB"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)
DBoDeallocMethod(SNLDB)

DBoLinkCreateMethod(SNLDB)
PyTypeObjectLinkPyType(SNLDB)
PyTypeObjectDefinitions(SNLDB)

}
