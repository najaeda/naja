#include "PySNLInstance.h"

#include "PySNLDesign.h"

#include "SNLInstance.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(Instance,instance,function)

static PyObject* PySNLInstance_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  PyObject* arg1 = nullptr;
  const char* arg2 = nullptr;
  if (not PyArg_ParseTuple(args, "OO|s:SNLDB.create", &arg0, &arg1, &arg2)) {
    setError("malformed SNLInstance create method");
    return nullptr;
  }
  SNLName name;
  if (arg2) {
    name = SNLName(arg2);
  }

  SNLInstance* instance = nullptr;
  SNLTRY
  if (not IsPySNLDesign(arg0)) {
    setError("SNLInstance create needs SNLDesign as first argument");
    return nullptr;
  }
  if (not IsPySNLDesign(arg1)) {
    setError("SNLInstance create needs SNLDesign as second argument");
    return nullptr;
  }
  instance = SNLInstance::create(PYSNLDesign_O(arg0), PYSNLDesign_O(arg1), name);
  SNLCATCH
  return PySNLInstance_Link(instance);
}

PyMethodDef PySNLInstance_Methods[] = {
  { "create", (PyCFunction)PySNLInstance_create, METH_VARARGS|METH_STATIC,
    "SNLInstance creator"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLInstance_destroy, PySNLInstance)
DBoDeallocMethod(SNLInstance)

DBoLinkCreateMethod(SNLInstance)
PyTypeObjectLinkPyType(SNLInstance)
PyTypeInheritedObjectDefinitions(SNLInstance, SNLDesignObject)

}
