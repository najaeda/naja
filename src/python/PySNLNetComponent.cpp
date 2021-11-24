#include "PySNLNetComponent.h"

#include "PySNLBitNet.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(SNLNetComponent, netComponent, function)

static PyObject* PySNLNetComponent_getNet(PySNLNetComponent* self) {
  METHOD_HEAD("SNLNetComponent.getNet()")
  SNL::SNLBitNet* net = netComponent->getNet();
  return PySNLBitNet_Link(net);
}

static PyObject* PySNLNetComponent_setNet(PySNLNetComponent* self, PyObject* arg) {
  METHOD_HEAD("SNLNetComponent.setNet()")
  if (IsPySNLBitNet(arg)) {
    netComponent->setNet(PYSNLBitNet_O(arg));
  } else {
    setError("SNLNetComponent getNet takes SNLBitNet argument");
    return nullptr;
  }
  Py_RETURN_NONE;
}

PyMethodDef PySNLNetComponent_Methods[] = {
  { "getNet", (PyCFunction)PySNLNetComponent_getNet, METH_NOARGS,
    "get SNLNetComponent SNLBitNet"},
  { "setNet", (PyCFunction)PySNLNetComponent_setNet, METH_O,
    "get SNLNetComponent SNLBitNet"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLNetComponent_destroy, PySNLNetComponent)
DBoDeallocMethod(SNLNetComponent)

DBoLinkCreateMethod(SNLNetComponent)
PyTypeObjectLinkPyType(SNLNetComponent)
PyTypeInheritedObjectDefinitions(SNLNetComponent, SNLDesignObject)

}
