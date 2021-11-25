#include "PySNLBusTermBit.h"

#include "PySNLDesign.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(Instance, instance, function)

PyMethodDef PySNLBusTermBit_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLBusTermBit_destroy, PySNLBusTermBit)
DBoDeallocMethod(SNLBusTermBit)

DBoLinkCreateMethod(SNLBusTermBit)
PyTypeObjectLinkPyType(SNLBusTermBit)
PyTypeObjectDefinitions(SNLBusTermBit)

}
