#include "PySNLBitNet.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(BitNet, net, function)

PyMethodDef PySNLBitNet_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLBitNet_destroy, PySNLBitNet)
DBoDeallocMethod(SNLBitNet)

DBoLinkCreateMethod(SNLBitNet)
PyTypeObjectLinkPyType(SNLBitNet)
PyTypeInheritedObjectDefinitions(SNLBitNet, SNLNet)

}
