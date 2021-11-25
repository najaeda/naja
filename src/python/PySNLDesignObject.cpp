#include "PySNLDesignObject.h"

#include "PySNLDesign.h"

namespace PYSNL {

using namespace SNL;

#define METHOD_HEAD(function) GENERIC_METHOD_HEAD(SNLDesignObject, designObject, function)

static PyObject* PySNLDesignObject_getDesign(PySNLDesignObject* self) {
  METHOD_HEAD("SNLDesignObject.getDesign()")
  return PySNLDesign_Link(designObject->getDesign());
}

PyMethodDef PySNLDesignObject_Methods[] = {
  {"getDesign", (PyCFunction)PySNLDesignObject_getDesign, METH_NOARGS,
    "Returns the SNLDesignObject owner design."},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLDesignObject_destroy, PySNLDesignObject)
DBoDeallocMethod(SNLDesignObject)

DBoLinkCreateMethod(SNLDesignObject)
PyTypeObjectLinkPyType(SNLDesignObject)
PyTypeObjectDefinitions(SNLDesignObject)

}
