#include "PySNLBusNet.h"

#include "PySNLDesign.h"

#include "SNLBusNet.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(BusNet, net, function)

static PyObject* PySNLBusNet_create(PyObject*, PyObject* args) {
  PyObject* arg0 = nullptr;
  int arg1 = 0;
  int arg2 = 0;
  const char* arg3 = nullptr;
  if (not PyArg_ParseTuple(args, "Oii|s:SNLDB.create", &arg0, &arg1, &arg2, &arg3)) {
    setError("malformed SNLBusNet create method");
    return nullptr;
  }
  SNLName name;
  if (arg3) {
    name = SNLName(arg3);
  }

  SNLBusNet* net = nullptr;
  SNLTRY
  if (IsPySNLDesign(arg0)) {
    net = SNLBusNet::create(PYSNLDesign_O(arg0), arg1, arg2, name);
  } else {
    setError("SNLBusNet create accepts SNLDesign as first argument");
    return nullptr;
  }
  SNLCATCH
  return PySNLBusNet_Link(net);
}

DirectGetIntMethod(PySNLBusNet_getMSB, getMSB, PySNLBusNet, SNLBusNet)
DirectGetIntMethod(PySNLBusNet_getLSB, getLSB, PySNLBusNet, SNLBusNet)

PyMethodDef PySNLBusNet_Methods[] = {
  { "create", (PyCFunction)PySNLBusNet_create, METH_VARARGS|METH_STATIC,
    "SNLBusNet creator"},
  { "getMSB", (PyCFunction)PySNLBusNet_getMSB, METH_NOARGS,
    "get SNLBusNet MSB value"},
  { "getLSB", (PyCFunction)PySNLBusNet_getLSB, METH_NOARGS,
    "get SNLBusNet LSB value"},
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLBusNet_destroy, PySNLBusNet)
DBoDeallocMethod(SNLBusNet)

DBoLinkCreateMethod(SNLBusNet)
PyTypeObjectLinkPyType(SNLBusNet)
PyTypeObjectDefinitions(SNLBusNet)

}
