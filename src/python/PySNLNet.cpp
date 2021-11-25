#include "PySNLNet.h"

#include "PySNLDesign.h"

namespace PYSNL {

using namespace SNL;

#undef   ACCESS_OBJECT
#undef   ACCESS_CLASS
#define  ACCESS_OBJECT            parent_.object_
#define  ACCESS_CLASS(_pyObject)  &(_pyObject->parent_)
#define  METHOD_HEAD(function)    GENERIC_METHOD_HEAD(Net, net, function)

PyMethodDef PySNLNet_Methods[] = {
  {NULL, NULL, 0, NULL}           /* sentinel */
};

DBoDestroyAttribute(PySNLNet_destroy, PySNLNet)
DBoDeallocMethod(SNLNet)

DBoLinkCreateMethod(SNLNet)
PyTypeObjectLinkPyType(SNLNet)
PyTypeInheritedObjectDefinitions(SNLNet, SNLDesignObject)

}
