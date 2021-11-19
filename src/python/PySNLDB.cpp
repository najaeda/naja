#include "PySNLDB.h"

#include "PySNLUniverse.h"
#include "PySNLLibrary.h"

#include "SNLDB.h"

namespace PYSNL {

using namespace SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDB,db,function)

static PyObject* PySNLDB_create(PyObject*, PyObject* args) {
  PyObject* arg = nullptr;
  if (not PyArg_ParseTuple(args, "O:SNLDB.create", &arg)) {
    //PyErr_SetString("");
    return nullptr;
  }
  if (not IsPySNLUniverse(arg)) {
    return nullptr;
  }
  auto universe = PYSNLUNIVERSE_O(arg);
  auto db = SNL::SNLDB::create(universe);
  return PySNLDB_Link(db);
}

static PyObject* PySNLDB_getLibrary(PySNLLibrary* self, PyObject* arg) {
  METHOD_HEAD("SNLLibrary.getLibrary()")
  SNLLibrary* subLibrary = NULL;

  if (PyUnicode_Check(arg)) {
    subLibrary = db->getLibrary(SNLName("toto"));
  } else {
    //PyErr_SetString
    return nullptr;
  }
  return PySNLLibrary_Link(subLibrary);
}

PyMethodDef PySNLDB_Methods[] = {
  { "create", (PyCFunction)PySNLDB_create, METH_VARARGS|METH_STATIC,
    "create a SNL DB"},
  { "getLibrary", (PyCFunction)PySNLDB_getLibrary, METH_O,
    "retrieve a SNL Library"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLDB_destroy, PySNLDB)
DBoDeallocMethod(SNLDB)

DBoLinkCreateMethod(SNLDB)
PyTypeObjectLinkPyType(SNLDB)
PyTypeObjectDefinitions(SNLDB)

}
