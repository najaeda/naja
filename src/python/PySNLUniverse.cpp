#include "PySNLUniverse.h"

#include "SNLUniverse.h"

namespace PYSNL {

using namespace SNL;

static PyObject* PySNLUniverse_create() {
  SNL::SNLUniverse* universe = nullptr;
  SNLTRY
  universe = SNL::SNLUniverse::create();
  SNLCATCH
  return PySNLUniverse_Link(universe);
}

static PyObject* PySNLUniverse_get() {
  auto universe = SNL::SNLUniverse::get();
  return PySNLUniverse_Link(universe);
}

PyMethodDef PySNLUniverse_Methods[] = {
  { "create", (PyCFunction)PySNLUniverse_create, METH_NOARGS|METH_STATIC,
    "create the SNL Universe (static object)"},
  { "get", (PyCFunction)PySNLUniverse_get, METH_NOARGS|METH_STATIC,
    "get the SNL Universe (static object)"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLUniverse_destroy, PySNLUniverse)
DBoDeallocMethod(SNLUniverse)

DBoLinkCreateMethod(SNLUniverse)
PyTypeObjectLinkPyType(SNLUniverse)
PyTypeObjectDefinitions(SNLUniverse)

}
