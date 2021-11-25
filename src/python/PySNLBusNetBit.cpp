#include "PySNLBusNetBit.h"

#include "PySNLDesign.h"

#include "SNLBusNetBit.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT           parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(BusNetBit, net, function)

PyMethodDef PySNLBusNetBit_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLBusNetBit_destroy, PySNLBusNetBit)
DBoDeallocMethod(SNLBusNetBit)

DBoLinkCreateMethod(SNLBusNetBit)
PyTypeObjectLinkPyType(SNLBusNetBit)
PyTypeObjectDefinitions(SNLBusNetBit)

}
